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
 *
 */

#include <os.h>
#include <pmm.h>
#include <pte.h>
#include <mmu.h>
#include <logical.h>
#include <cpu.h>
#include <hcall.h>

sval
hpte_scan_unmap(struct logical_chunk_info *lci)
{
	if (lci->lci_ptecount != 0) {
		if (lci->lci_os->cached_partition_info[1].sfw_tlb) {
			tlbia();
			ptesync();
		} else {
			ptes_tlbies(lci->lci_os, lci);
#if BLOW_AWAY_THE_TLB
			tlbia();
#endif
		}
		lci->lci_ptecount = 0;
	}
	return 0;
}

sval phys_mem_unmap(struct logical_chunk_info *lci)
	__attribute__ ((alias("hpte_scan_unmap")));

static uval
phys_mem_amo_check_WIMG(struct logical_chunk_info *lci, uval laddr,
			union ptel ptel)
{
	(void)laddr;

	switch (lci->lci_res.sr_type) {
	case MEM_ADDR:
		/* b0010 is valid */
		if ((ptel.bits.w == 0) &&
		    (ptel.bits.i == 0) &&
		    (ptel.bits.m == 1) && (ptel.bits.g == 0)) {
			return 1;
		}
		break;
	case MMIO_ADDR:
		/* b01*1 is valid */
		if ((ptel.bits.w == 0) &&
		    (ptel.bits.i == 1) && (ptel.bits.g == 1)) {
			return 1;
		}
		break;
	}

#ifdef DEBUG
	hprintf("pmm: WIMG bits incorrect for memory type "
		"w=%x i=%d m=%d, g=%d\n word 0x%llx\n",
		ptel.bits.w, ptel.bits.i, ptel.bits.m, ptel.bits.g, ptel.word);
#endif
	return 0;
}

struct arch_mem_ops pmem_ops = {
	.amo_check_WIMG = phys_mem_amo_check_WIMG,
};
