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

#include <lib.h>
#include <hype.h>
#include <os.h>
#include <pmm.h>
#include <mmu.h>
#include <cpu.h>
#include <vm.h>

/*
 * Create a simple linear to physical mapping.  Allocate a virtual address
 * for the mapping if vaddr passed in is 0, otherwise used passed in
 * vaddr. This code assumes that all page tables are allocated
 * and it should therefore only be used on the hv_pgd for which we
 * can guarantee this.
 *
 * Returns the virtual address, or zero if get_pages returned none.
 */
uval
hv_map(uval vaddr, uval paddr, uval size, uval flags, uval flush)
{
	uval pdi;
	uval pti;
	uval mapped = 0;

	if (vaddr == 0) {
		vaddr = get_pages(&phys_pa, size);
		if (vaddr == PAGE_ALLOC_ERROR) {
			return PAGE_ALLOC_ERROR;
		}
	}
	pdi = (vaddr & PDE_MASK) >> LOG_PDSIZE;
	pti = (vaddr & PTE_MASK) >> LOG_PGSIZE;

	assert((vaddr & (PGSIZE - 1)) == 0, "vaddr not page-aligned");
	assert((paddr & (PGSIZE - 1)) == 0, "paddr not page-aligned");
	assert(vaddr >= HV_VBASE &&
	       vaddr <= HV_VBASE + HV_VSIZE - 1, "bad vaddr");

#ifdef VM_DEBUG
	hprintf("Mapping 0x%lx bytes from 0x%lx paddr to 0x%lx virtual\n",
		size, paddr, vaddr);
#endif

	for (; mapped < size; pti = 0, pdi++) {
		uval pde = hv_pgd[pdi];

		assert((pde & PTE_P), "PDE not present");
		assert(!(pde & PTE_PS), "PDE large page");

		uval *pgtab = (uval *)virtual(pde & PGMASK);

		for (; pti < 1024 && mapped < size; pti++, mapped += PGSIZE)
			pgtab[pti] = (paddr + mapped) | PTE_P | flags;
	}

	if (flush) {
		invalidate_tlb(vaddr, size);
	}
	return vaddr;
}

void
hv_unmap(uval vaddr, uval size)
{
	hv_map(vaddr, (vaddr - HV_VBASE), size, PTE_RW, 1);
	free_pages(&phys_pa, vaddr, size);
}

/*
 * Translate HV physical to virtual address
 */
uval
virtual(const uval paddr)
{
	assert(paddr < HV_VSIZE, "bad paddr");
	return paddr + HV_VBASE;
}

/*
 * Translate HV virtual to physical address
 */
uval
physical(uval vaddr)
{
	int pdi = (vaddr & PDE_MASK) >> LOG_PDSIZE;
	uval pde = hv_pgd[pdi];

	if (vaddr < HV_VBASE || vaddr > (HV_VBASE + HV_VSIZE - 1)) {
		return INVALID_VIRTUAL_ADDRESS;
	}

	if ((pde & PTE_P) == 0)
		return INVALID_VIRTUAL_ADDRESS;
	if (pde & PTE_PS)
		return (pde & PTE_LARGEBASE) | (vaddr & ~PTE_LARGEBASE);

	int pti = (vaddr & PTE_MASK) >> LOG_PGSIZE;
	uval *pgtab = (uval *)virtual(pde & PGMASK);
	uval pte = pgtab[pti];

	if ((pte & PTE_P) == 0)
		return INVALID_VIRTUAL_ADDRESS;

	return (pte & PTE_SMALLBASE) | (vaddr & ~PTE_SMALLBASE);
}

/*
 * Invalidate TLB
 */
void
invalidate_tlb(uval vaddr, uval len)
{
	uval end = vaddr + len - 1;

	while (vaddr <= end) {
		/* *INDENT-OFF* */
		__asm__ __volatile__("invlpg %0" :: "m" (*(char *)vaddr));
		/* *INDENT-ON* */
		vaddr += PGSIZE;
	}
}

/*
 * Get shadow PTE entry for this virtual address,
 * or 0 if the PTE entry does not exists.
 */
static uval
getpte(struct cpu_thread *thread, const uval vaddr, int *large)
{
	union pgframe *pgd = thread->pgd;
	union pgframe *pgt;
	int pdi;
	int pti;
	uval pde;

	assert(vaddr < HV_VBASE, "bad vaddr");

	pdi = (vaddr & PDE_MASK) >> LOG_PDSIZE;

	pde = pgd->pgdir.hv_vaddr[pdi];
	if ((pde & PTE_P) == 0) {
		/* page directory entry not present */
		return 0;
	}

	if (pde & PTE_PS) {
		/* 4MB page */
		*large = 1;
		return pde;
	}

	/* page table pointer */
	*large = 0;
	pgt = pgd->pgdir.pgt[pdi];
	assert(pgt != NULL, "null pgt");

	pti = (vaddr & PTE_MASK) >> LOG_PGSIZE;
	return pgt->pgtab.hv_vaddr[pti];
}

/*
 * Convert a virtual address to physical, given a thread
 * context.  Return -1 if the address does not exists or
 * is not present.
 */
uval
v2p(struct cpu_thread *thread, const uval vaddr)
{
	int large;
	uval pte;

	pte = getpte(thread, vaddr, &large);
	if ((pte & PTE_P) == 0) {
		return (uval)-1;
	}

	if (large) {
		return (pte & PTE_LARGEBASE) | (vaddr & ~PTE_LARGEBASE);
	} else {
		return (pte & PTE_SMALLBASE) | (vaddr & ~PTE_SMALLBASE);
	}
}

/*
 * Determine wether the address range is backed up by page
 * mapping with the appropriate mask bits set.
 */
int
ispresent(struct cpu_thread *thread, uval vaddr, uval len, uval mask,
	  uval *error, uval *fault)
{
	uval end = vaddr + len - 1;
	int large;

	while (vaddr <= end) {
		uval pte = getpte(thread, vaddr, &large);

		if ((pte & PTE_P) == 0) {
			*fault = vaddr;
			*error = mask & ~PTE_P;
			return 0;
		}
		if ((pte & mask) != mask) {
			*fault = vaddr;
			*error = mask | PTE_P;
			return 0;
		}
		if (large) {
			vaddr += LPGSIZE;
		} else {
			vaddr += PGSIZE;
		}
	}
	return 1;
}

/*
 * Determine whether n words of stack space is available
 * for the specified thread.
 */
int
isstackpresent(struct cpu_thread *thread, uval n, uval mask, uval *error,
	       uval *fault)
{
	uval len = n * sizeof (uval);
	uval bottom = thread->tss.gprs.regs.esp - len;

	return ispresent(thread, bottom, len, mask, error, fault);
}
