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

#include <config.h>
#include <lib.h>
#include <os.h>
#include <hype.h>
#include <mmu.h>
#include <pmm.h>
#include <vm.h>
#include <debug.h>
#include <pgcache.h>
#include <h_proto.h>

#undef PGE_DEBUG

/*
 * Combine the flags of a shadow PTE with the partition PTE. This
 * allows partitions to query the access/modify bits.
 */
static inline uval
combine(uval lpe, uval hve)
{
	if (hve & PTE_P)
		return (lpe & ~PTE_FLAGS) | (hve & PTE_FLAGS);
	else
		return lpe;
}

/*
 * Return the Root/PDE/PTE entry that is stored in the current shadow page
 * table, or if the shadow page table is empty, stored in the partition
 * page table. 
 *
 * ``flags'' are defined as:
 *
 *	H_GET_ENTRY_ROOT	Return pointer to the current PDE
 *
 *	H_GET_ENTRY_PDE		Return PDE for specified ``vaddr''
 *
 *	H_GET_ENTRY_PTE		Return PTE for specified ``vaddr''
 *
 * When H_GET_ENTRY_PHYSICAL is or-ed in to the flags, the physical address
 * for the Root/PDE/PTE is returned rather than the logical address.
 */
sval
h_get_pte(struct cpu_thread *thread, uval flags, uval vaddr)
{
	union pgframe *pgd = thread->pgd;

#ifdef PGE_DEBUG
	hprintf("H_GET_PTE: flags {");
	if (flags & H_GET_ENTRY_ROOT)
		hprintf(" ROOT");
	if (flags & H_GET_ENTRY_PDE)
		hprintf(" PDE");
	if (flags & H_GET_ENTRY_PTE)
		hprintf(" PTE");
	if (flags & H_GET_ENTRY_PHYSICAL)
		hprintf(" Physical");
	hprintf(" }, vaddr 0x%lx\n", vaddr);
#endif

	if (vaddr >= HV_VBASE)
		return H_NOT_FOUND;

	/* Read a root directory entry */
	if (flags & H_GET_ENTRY_ROOT) {
		if (flags & H_GET_ENTRY_PHYSICAL) {
			return_arg(thread, 1, pgd->pgdir.hv_paddr);
		} else {
			/* logical */
			return_arg(thread, 1, pgd->pgdir.lp_laddr);
		}

		return H_Success;
	}

	uval pdi = (vaddr & PDE_MASK) >> LOG_PDSIZE;

	/* Read a page directory entry */
	if (flags & H_GET_ENTRY_PDE) {
		uval lp_pde = pgd->pgdir.lp_vaddr[pdi];
		uval hv_pde = pgd->pgdir.hv_vaddr[pdi];

		if ((lp_pde & PTE_P) == 0)
			return H_NOT_FOUND;

		if (flags & H_GET_ENTRY_PHYSICAL) {
			/* no LPAR pde for this address */
			if ((lp_pde & PTE_P) == 0)
				return H_NOT_FOUND;

			/* no HV pde for this address, create one */
			if ((hv_pde & PTE_P) == 0) {
				assert(0, "not yet");
			}

			/* hv_pde may have changed */
			return_arg(thread, 1, pgd->pgdir.hv_vaddr[pdi]);
		} else {
			/* logical */
			return_arg(thread, 1, combine(lp_pde, hv_pde));
		}

		return H_Success;
	}

	uval pti = (vaddr & PTE_MASK) >> LOG_PGSIZE;

	/* Read a page table entry */
	if (flags & H_GET_ENTRY_PTE) {
		uval lp_pde = pgd->pgdir.lp_vaddr[pdi];
		uval hv_pde = pgd->pgdir.hv_vaddr[pdi];

		if ((lp_pde & PTE_P) == 0)
			return H_NOT_FOUND;

		if (lp_pde & PTE_PS) {
			/* large page */
			if (flags & H_GET_ENTRY_PHYSICAL) {
				/* no LPAR pde for this address */
				if ((lp_pde & PTE_P) == 0)
					return H_NOT_FOUND;

				/* no HV pde for this address, create one */
				if ((hv_pde & PTE_P) == 0) {
					assert(0, "not yet");
				}

				/* hv_pde may have changed */
				return_arg(thread, 1,
					   pgd->pgdir.hv_vaddr[pdi]);
			} else {
				/* logical */
				return_arg(thread, 1, combine(lp_pde, hv_pde));
			}

			return H_Success;
		}

		union pgframe *pgt = pgd->pgdir.pgt[pdi];
		uval lp_pte = pgt->pgtab.lp_vaddr[pti];
		uval hv_pte = pgt->pgtab.hv_vaddr[pti];

		if ((lp_pte & PTE_P) == 0)
			return H_NOT_FOUND;

		if (flags & H_GET_ENTRY_PHYSICAL) {
			/* no LPAR pte for this address */
			if ((lp_pte & PTE_P) == 0)
				return H_NOT_FOUND;

			/* no HV pte for this address, create one */
			if ((hv_pte & PTE_P) == 0) {
				if (!pgc_set_pte(thread, pgt, pti, lp_pte))
					return H_NOT_FOUND;
			}

			/* hv_pte may have changed */
			return_arg(thread, 1, pgt->pgtab.hv_vaddr[pti]);
		} else {
			/* logical */
			return_arg(thread, 1, combine(lp_pte, hv_pte));
		}

		return H_Success;
	}

	return H_Parameter;
}
