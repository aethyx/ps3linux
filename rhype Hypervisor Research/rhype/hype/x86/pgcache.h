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
#ifndef __HYPE_X86_PGCACHE_H__
#define __HYPE_X86_PGCACHE_H__

#include <mmu.h>
#include <pmm.h>

/*
 * Currently, 20% of the allocated frames a directory frames.
 * The others are page table frames.
 *
 * For each context, we need 1 page PDE (32B + 3*4KB)
 * and (32B + 2*4KB) per PTE.
 *
 * The total amount of "space" required per page cache is:
 *
 *	NR_DIR_FRAMES * (32B + 3*4KB) +
 *	(NR_FRAMES-NR_DIR_FRAMES) * (32B + 2*4KB)
 */
#define NR_FRAMES	(4 * PGSIZE/sizeof(union pgframe))
#define	NR_DIR_FRAMES	((NR_FRAMES*20)/100)

/*
 * Quickly lookup a frame of give type.
 * This should probably be replaced by a more cache friendly data structure.
 */
static inline union pgframe *
pgc_lookup(struct pgcache *pgc, uval laddr, int type)
{
	union pgframe *h;

	assert(type >= PGC_PGDIR && type < PGC_NTYPES, "out of bounds");

	for (h = pgc->htab[type][HASH(laddr)]; h; h = h->cmn.next)
		if (h->cmn.lp_laddr == laddr)
			break;
	return h;
}

/*
 * Setup the page tables
 */
static inline void
pgc_set_pgtab(union pgframe *pgd, uval pdi, union pgframe *pgtab, uval flags)
{
	pgd->pgdir.hv_vaddr[pdi] = pgtab->pgtab.hv_paddr | flags;
	pgd->pgdir.pgt[pdi] = pgtab;
	pgtab->pgtab.pdi = pdi;
	pgtab->pgtab.pgd = pgd;
}

/* ISA memory mapped I/O space */
#define	isa_iospace(a)	((a) >= 0x0A0000 && (a) < 0x100000)

/*
 * Set a hypervisor (shadow) page table entry, where the PTE entry
 * given is in logical format.
 *
 * For the logical to physical address conversion the following
 * rules are used (in order):
 *
 *	- If the logical address is available to the partition,
 *	  its corresponding physical address is used
 *
 *	- If the partition is running at IOPL2 and the logical
 *	  address does not refer to a valid physical memory address,
 *	  the logical address is used verbatim. This mechanism
 *	  allows memory mapped devices to be used in the I/O
 *	  partition.
 */
static inline int
pgc_set_pte(struct cpu_thread *thread, union pgframe *table,
	    uval index, uval32 entry)
{
	uval ioprivilege = (thread->cpu->os->iopl == EFLAGS_IOPL_2);
	uval laddr = pte_pageaddr(entry);

	if (entry & PTE_P) {
		uval paddr = logical_to_physical_address(thread->cpu->os,
							 laddr, PGSIZE);

		if (unlikely(paddr == INVALID_PHYSICAL_ADDRESS)) {
			if (ioprivilege && laddr >= max_phys_memory) {
				/* it is a memory mapped I/O address */
				table->pgtab.hv_vaddr[index] = entry;
				return 1;
			}
			table->pgtab.hv_vaddr[index] = 0;	/* invalid */
			return 0;
		}

		if (unlikely(ioprivilege && isa_iospace(laddr))) {
			/* it is an ISA memory mapped I/O address */
			table->pgtab.hv_vaddr[index] = entry;
			return 1;
		}

		uval32 pte = paddr | pte_flags(entry);

		/* reflect dirty bit to partition by double faulting */
		if ((pte & PTE_D) == 0)
			pte &= ~PTE_RW;

		table->pgtab.hv_vaddr[index] = pte;
	} else			/* invalidate the entry */
		table->pgtab.hv_vaddr[index] = 0;

	return 1;
}

extern void pgc_init(struct os *);
extern void pgc_hypervisor_mapping(union pgframe *);
extern void pgc_initial_mapping(struct cpu_thread *);
extern union pgframe *pgc_alloc(struct cpu_thread *, uval, int);
extern void pgc_free(struct pgcache *, union pgframe *, int);
extern void pgc_flush(struct cpu_thread *, union pgframe *);
extern uval pgc_prefetch(struct cpu_thread *, union pgframe *, int);
extern void pgc_destroy(struct os *);

#ifdef DEBUG
extern void pgc_dump(union pgframe *, int);
#endif

#endif /* __HYPE_X86_PGCACHE_H__ */
