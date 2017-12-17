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
 * Implement various kinds of TLB flushes.
 */

#include <config.h>
#include <lib.h>
#include <cpu_thread.h>
#include <mmu.h>
#include <pmm.h>
#include <vm.h>
#include <debug.h>
#include <pgcache.h>
#include <h_proto.h>

#undef FLSH_DEBUG

/*
 * Flush a single TLB entry
 */
static uval
flush_single(struct cpu_thread *thread, uval vaddr, int prefetch)
{
	uval pdi = (vaddr & PDE_MASK) >> LOG_PDSIZE;
	uval pti = (vaddr & PTE_MASK) >> LOG_PGSIZE;

#ifdef FLSH_DEBUG
	hprintf("H_FLUSH_TLB: flush_single: flushing 0x%lx (prefetch %d)\n",
		vaddr, prefetch);
#endif

	if (vaddr >= HV_VBASE)
		return H_Parameter;

	/*
	 * If we don't have the flushed address cache, we go ahead and
	 * invalidate it anyway. If we do have it cache, we need to update
	 * our cache, possibly refetching the pde/pte entry from the
	 * the guest.
	 */
	uval pde = thread->pgd->pgdir.hv_vaddr[pdi];

	if ((pde & (PTE_PS | PTE_P)) == PTE_P) {
		union pgframe *pgtable = thread->pgd->pgdir.pgt[pdi];
		uval pte = prefetch ? pgtable->pgtab.lp_vaddr[pti] : 0;

		if (!pgc_set_pte(thread, pgtable, pti, pte))
			return H_Parameter;
	} else if ((pde & (PTE_PS | PTE_P)) == (PTE_PS | PTE_P)) {
		/* large page */
		uval pde = prefetch ? thread->pgd->pgdir.lp_vaddr[pti] : 0;

		if (!pgc_set_pte(thread, thread->pgd, pdi, pde))
			return H_Parameter;
	}

	/* finally do the invalidation */
	/* *INDENT-OFF* */
	__asm__ __volatile__ ("invlpg %0"
			      : /* output */
			      : "m"(*(uval *)vaddr)
			      : "memory");
	/* *INDENT-ON* */

	/* no need to update shadow tables anymore */
	if (prefetch && thread->cb && thread->reg_cr2 == vaddr)
		thread->cb = NULL;

	return H_Success;
}

/*
 * Flush all TLB entries
 */
static uval
flush_all(struct cpu_thread *thread, int prefetch)
{
	uval tmp;

#ifdef FLSH_DEBUG
	hprintf("H_FLUSH_TLB: flush_all: flushing lpid 0x%x (prefetch %d)\n",
		thread->cpu->os->po_lpid, prefetch);
#endif

	/* flush all shadow page table copies that are currently active */
	if (prefetch)
		pgc_prefetch(thread, thread->pgd, PGC_PGDIR);
	else
		pgc_flush(thread, thread->pgd);

	/* flush whole TLB */
	/* *INDENT-OFF* */
        __asm__ __volatile__ (
		"movl %%cr3, %0; movl %0, %%cr3"
		: "=r" (tmp)
		: /* input */
		: "memory"
	);
	/* *INDENT-ON* */

	/* no need to update shadow tables anymore */
	if (prefetch && thread->cb)
		thread->cb = NULL;

	return H_Success;
}

/*
 * Flush all Global TLB entries
 */
static uval
flush_global(struct cpu_thread *thread, int prefetch)
{
	uval tmp;

	/* flush all page table copies that are currently active */
	if (prefetch)
		pgc_prefetch(thread, thread->pgd, PGC_PGDIR);
	else
		pgc_flush(thread, thread->pgd);

	/* turn off PGE, flush TLB, turn on PGE (see section 3.11) */
	/* *INDENT-OFF* */
	__asm__ __volatile__ (
		"movl %1, %%cr4;"
		"movl %%cr3, %0;"
		"movl %0, %%cr3;"
		"movl %2, %%cr4"
		: "=&r" (tmp)
		: "r" (thread->reg_cr4 & ~CR4_PGE), "r" (thread->reg_cr4)
		: "memory"); 
	/* *INDENT-ON* */

	/* no need to update shadow tables anymore */
	if (prefetch && thread->cb)
		thread->cb = NULL;

	return H_Success;
}

/*
 * The consistency of the hypervisor shadow page tables and the CPU's TLB is
 * managed through H_FLUSH_TLB().
 *
 * ``flags'' are defined as:
 *
 *	H_FLUSH_TLB_SINGLE	Flush a single page, as identified by
 *				``vaddr'' from the current shadow page
 *				table and flush its corresponding TLB
 *				entry.
 *
 *	H_FLUSH_TLB_ALL		Flush all pages from the shadow page table
 *				cache and flush the entire TLB.
 *
 *	H_FLUSH_TLB_GLOBAL	Flush all global pages from the shadow page
 *				table cache and flush the TLB entries.
 *
 *	H_FLUSH_TLB_PREFETCH	After a flush, prefetch the flushed mapping(s)
 *				from the guest page tables. This can either
 *				be a single entry for H_FLUSH_TLB_SINGLE or
 *				the whole page table in the event of
 *				H_FLUSH_TLB_ALL.
 */
sval
h_flush_tlb(struct cpu_thread *thread, uval flags, uval vaddr)
{
	int prefetch = flags & H_FLUSH_TLB_PREFETCH;

	if (flags & H_FLUSH_TLB_SINGLE)
		return flush_single(thread, vaddr, prefetch);
	if (flags & H_FLUSH_TLB_ALL)
		return flush_all(thread, prefetch);
	if (flags & H_FLUSH_TLB_GLOBAL)
		return flush_global(thread, prefetch);

	return H_Parameter;
}
