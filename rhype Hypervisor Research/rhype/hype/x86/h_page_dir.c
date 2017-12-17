/*
 * Copyright (C) 2005 Jimi Xenidis <jimix@watson.ibm.com>, IBM Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id$
 */
/*
 *
 * Implement h_page_dir and page fault handling.
 *
 */

#include <config.h>
#include <lib.h>
#include <cpu_thread.h>
#include <pgalloc.h>
#include <mmu.h>
#include <pmm.h>
#include <vm.h>
#include <idt.h>
#include <debug.h>
#include <timer.h>
#include <pgcache.h>
#include <h_proto.h>

#undef PGF_DEBUG

/*
 * Actual context switch
 */
static inline void
activate(struct cpu_thread *thread, union pgframe *p)
{
	thread->pgd = p;
	thread->tss.cr3 = p->pgdir.hv_paddr;
	p->pgdir.last = ticks;
}

/*
 * Creating a new shadow page table context is achieved through H_PAGE_DIR().
 * This call also controls the context switching between shadow page table
 * maps and potential prefetching of the shadow page tables kept by the
 * hypervisor. Prefetching is prefered over faulting page mappings in on
 * individual hypervisor page faults.
 *
 * ``flags'' are defined as:
 *
 *	H_PAGE_DIR_FLUSH	Flush all shadow (cached) page table entries
 *				for this ``lp_pgdir_laddr''. If this is the
 *				current map, the map is guaranteed to stay
 *				active until the next H_PAGE_DIR_ACTIVATE.
 *
 *	H_PAGE_DIR_PREFETCH     Prefetch all the shadow (cached) page table
 *				entries for this ``lp_pgdir_laddr'' from the
 *				guest partition.
 *
 *	H_PAGE_DIR_ACTIVATE	Activate the current context, as indicated
 *				by the page table root ``lp_pgdir_laddr''.
 *
 *	Bitwise combinations of these flags are allowed.
 *
 * ``lp_pgdir_laddr'' is defined as the logical address of the partition's
 * page table root directory.
 */
sval
h_page_dir(struct cpu_thread *thread, uval flags, uval lp_pgdir_laddr)
{
	struct pgcache *pgc = &thread->cpu->os->pgcache;
	union pgframe *pgd = pgc_lookup(pgc, lp_pgdir_laddr, PGC_PGDIR);

#ifdef PGF_DEBUG
	hprintf("H_PAGE_DIR: flags {");
	if (flags & H_PAGE_DIR_FLUSH)
		hprintf(" FLUSH");
	if (flags & H_PAGE_DIR_PREFETCH)
		hprintf(" PREFETCH");
	if (flags & H_PAGE_DIR_ACTIVATE)
		hprintf(" ACTIVATE");
	hprintf(" }, pgdir 0x%lx\n", lp_pgdir_laddr);
#endif

	/* the most frequent operation is a simple context switch */
	if (likely(pgd && flags == H_PAGE_DIR_ACTIVATE)) {
		activate(thread, pgd);
		return H_Success;
	}

	/* if necessary, allocate a page cache entry for this pgdir */
	if (pgd == NULL) {
		pgd = pgc_alloc(thread, lp_pgdir_laddr, PGC_PGDIR);
		if (pgd == NULL)
			return H_Parameter;
		pgc_hypervisor_mapping(pgd);
		flags |= H_PAGE_DIR_PREFETCH;
	}

	/* process flush/prefetch hints, if any */
	switch (flags & (H_PAGE_DIR_FLUSH | H_PAGE_DIR_PREFETCH)) {
	case H_PAGE_DIR_FLUSH:
		/* flush the entire pgdir on the next activate */
		if (thread->flushed && thread->flushed != pgd)
			pgc_free(pgc, thread->flushed, PGC_PGDIR);
		thread->flushed = pgd;
		break;

	case H_PAGE_DIR_PREFETCH:
	case H_PAGE_DIR_FLUSH | H_PAGE_DIR_PREFETCH:
		/* prefetch any entries if so desired */
		pgc_prefetch(thread, pgd, PGC_PGDIR);
		break;
	}

	/* activate this page directory */
	if (flags & H_PAGE_DIR_ACTIVATE)
		activate(thread, pgd);

	return H_Success;
}

#ifdef PGF_DEBUG
static void
print_fault(struct cpu_thread *thread, uval32 error_code, uval faulting_addr)
{
	static uval faultcount = 0;

	hprintf("PAGE FAULT (%ld) AT lpid 0x%x, address 0x%lx "
		"(error code 0x%x, eip: 0x%lx) - ",
		faultcount++, thread->cpu->os->po_lpid,
		faulting_addr, error_code, thread->tss.eip);

	/* PTE_P */
	if (error_code & 1) {
		hprintf("Present ");
	} else {
		hprintf("Absent ");
	}

	/* PTE_RW */
	if (error_code & 2) {
		hprintf("Write ");
	} else {
		hprintf("Read ");
	}

	/* PTE_US */
	if (error_code & 4) {
		hprintf("User ");
	} else {
		hprintf("Supervisor ");
	}

	if (error_code & 8) {
		hprintf("RSVD\n");
	} else {
		hprintf("Normal\n");
	}
}
#endif /* PGF_DEBUG */

/*
 * The routine handles the exceptional case where the partition
 * has not yet established a page table. In that case all available
 * physical memory is mapped linearly into the partition. We use
 * large page table entries to handle this.
 */
static int
linear_fault(struct cpu_thread *thread, uval32 error_code, uval faulting_addr)
{
	uval pdi = (faulting_addr & PDE_MASK) >> LOG_PDSIZE;

	uval32 hv_pde = thread->pgd->pgdir.hv_vaddr[pdi];

	assert((hv_pde & PTE_P) == 0, "Unexpected linear page fault");

	uval32 lp_pde = (faulting_addr & PTE_LARGEBASE)
		| PTE_PS | PTE_RW | PTE_A | PTE_D | PTE_P;

	if (!pgc_set_pte(thread, thread->pgd, pdi, lp_pde)) {
		force_page_fault(thread, error_code, faulting_addr);
		return 1;
	}

	return 0;
}

/*
 * This routine is called upon a page fault call back into the
 * hypervisor. Most likely, the guest modified the page table
 * as a result of the page fault. In this routine we handle the
 * simple updates to the shadow page table so that we prevent
 * double page faults.
 */
static void
shadow_fault_callback(struct cpu_thread *thread)
{
	union pgframe *pgd = thread->pgd;
	union pgframe *pgt;

	thread->cb = NULL;

	uval pdi = (thread->reg_cr2 & PDE_MASK) >> LOG_PDSIZE;
	uval pti = (thread->reg_cr2 & PTE_MASK) >> LOG_PGSIZE;

	uval32 lp_pde = pgd->pgdir.lp_vaddr[pdi];

	/* partition page directory not present */
	if ((lp_pde & PTE_P) == 0)
		return;

	/* handle large page */
	if (lp_pde & PTE_PS) {
		pgc_set_pte(thread, pgd, pdi, lp_pde);
		return;
	}

	/* get page table directory */
	if ((pgt = pgd->pgdir.pgt[pdi]) == NULL) {
		/* create new page cache entry for this page directory */
		pgt = pgc_alloc(thread, pte_pageaddr(lp_pde), PGC_PGTAB);
		if (pgt == NULL)
			return;

		/* prefetch this page table */
		pgc_set_pgtab(pgd, pdi, pgt, pte_flags(lp_pde));
		pgc_prefetch(thread, pgt, PGC_PGTAB);
	}

	uval32 lp_pte = pgt->pgtab.lp_vaddr[pti];

	/* partition page table not present */
	if ((lp_pte & PTE_P) == 0)
		return;

	/* set new shadow PTE entry or update flags */
	pgc_set_pte(thread, pgt, pti, lp_pte);
}

/*
 * Shadow page table handling
 */
static int
shadow_fault(struct cpu_thread *thread, uval32 error_code, uval faulting_addr)
{
	union pgframe *pgt;

	uval32 mask = error_code & 0x7;

	uval pdi = (faulting_addr & PDE_MASK) >> LOG_PDSIZE;
	uval pti = (faulting_addr & PTE_MASK) >> LOG_PGSIZE;

	uval32 lp_pde = thread->pgd->pgdir.lp_vaddr[pdi];
	uval32 hv_pde = thread->pgd->pgdir.hv_vaddr[pdi];

	/* access violation on page directory entry */
	if ((lp_pde & mask) != mask) {
		if (!force_page_fault(thread, error_code, faulting_addr))
			thread->cb = shadow_fault_callback;
		return 1;
	}

	/* page directory is not present */
	if ((hv_pde & PTE_P) == 0) {
		if ((lp_pde & PTE_P) == 0) {
			if (!force_page_fault(thread, error_code,
					      faulting_addr)) {
				thread->cb = shadow_fault_callback;
			}
			return 1;
		}

		/* handle large pages seperately */
		if (lp_pde & PTE_PS) {
			if (!pgc_set_pte(thread, thread->pgd, pdi, lp_pde)) {
				if (!force_page_fault(thread, error_code,
						      faulting_addr)) {
					thread->cb = shadow_fault_callback;
				}
				return 1;
			}
			return 0;
		}

		/* create new page cache entry for this page directory */
		pgt = pgc_alloc(thread, pte_pageaddr(lp_pde), PGC_PGTAB);

		/* prefetch this page table */
		if (pgt != NULL) {
			pgc_set_pgtab(thread->pgd, pdi, pgt,
				      pte_flags(lp_pde));
			pgc_prefetch(thread, pgt, PGC_PGTAB);
		}
	} else
		pgt = thread->pgd->pgdir.pgt[pdi];

	/* large page is written to, reflect dirty bit back to the partition */
	if (unlikely((mask & PTE_RW) && (hv_pde & PTE_RW) == 0 &&
		     (lp_pde & (PTE_PS | PTE_RW)) == (PTE_PS | PTE_RW))) {
		thread->pgd->pgdir.lp_vaddr[pdi] |= PTE_A | PTE_D;
		if (!pgc_set_pte(thread, thread->pgd, pdi,
				 lp_pde | PTE_A | PTE_D)) {
			if (!force_page_fault(thread, error_code,
					      faulting_addr)) {
				thread->cb = shadow_fault_callback;
			}
			return 1;
		}
		return 0;
	}

	if (pgt == NULL) {
		if (!force_page_fault(thread, error_code, faulting_addr))
			thread->cb = shadow_fault_callback;
		return 1;
	}

	uval32 lp_pte = pgt->pgtab.lp_vaddr[pti];
	uval32 hv_pte = pgt->pgtab.hv_vaddr[pti];

	/* if the access doesn't match the PTE flags, forward fault */
	if ((lp_pte & mask) != mask) {
		if (!force_page_fault(thread, error_code, faulting_addr))
			thread->cb = shadow_fault_callback;
		return 1;
	}

	/* page table is not present */
	if ((hv_pte & PTE_P) == 0 && (lp_pte & PTE_P) == 0) {
		if (!force_page_fault(thread, error_code, faulting_addr))
			thread->cb = shadow_fault_callback;
		return 1;
	}

	/*
	 * Initially when pages are mapped for writing we mask off the
	 * RW bit so that the first write will cause a page fault. This
	 * allows us to set the dirty bit in the partition's page table,
	 * whenever a page is writen.
	 */
	if (unlikely((mask & PTE_RW) && (hv_pte & (PTE_RW | PTE_P)) == PTE_P &&
		     (lp_pte & PTE_RW))) {
		lp_pte |= PTE_A | PTE_D;
		pgt->pgtab.lp_vaddr[pti] |= PTE_A | PTE_D;
	}

	/* set new shadow PTE entry or update flags */
	if (!pgc_set_pte(thread, pgt, pti, lp_pte)) {
		if (!force_page_fault(thread, error_code, faulting_addr))
			thread->cb = shadow_fault_callback;
		return 1;
	}

	return 0;
}

/*
 * Hypervisor page fault handler
 */
int
page_fault(struct cpu_thread *thread, uval32 error_code, uval faulting_addr)
{
#ifdef PGF_DEBUG
	print_fault(thread, error_code, faulting_addr);
#endif

	/* the hypervisor should never fault */
	if (unlikely((thread->tss.srs.regs.cs & 0x3) == 0)) {
		hprintf("Page fault in the hypervisor: address 0x%lx\n",
			faulting_addr);
#ifdef DEBUG
		dump_cpu_state(thread);
#endif
		assert(0, "page fault in HV\n");
		return 1;
	}

	/* LPAR mistakenly set reserved bit; nothing we can do here */
	if (unlikely(error_code & 0x8)) {
		force_page_fault(thread, error_code, faulting_addr);
		return 1;
	}

	/* if we haven't established a page table yet, handle it specially */
	if (unlikely(thread->pgd->pgdir.lp_laddr == INVALID_PHYSICAL_ADDRESS))
		return linear_fault(thread, error_code, faulting_addr);
	else
		return shadow_fault(thread, error_code, faulting_addr);
}

/*
 * Get page fault address
 */
sval
h_get_pfault_addr(struct cpu_thread *thread)
{
	return_arg(thread, 1, thread->reg_cr2);
	return H_Success;
}
