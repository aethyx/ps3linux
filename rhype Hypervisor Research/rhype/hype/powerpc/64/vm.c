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
#include <lpar.h>
#include <mmu.h>
#include <htab.h>
#include <types.h>
#include <hype.h>
#include <logical.h>
#include <pmm.h>
#include <thread_control_area.h>
#include <mmu.h>
#include <pte.h>
#include <h_proto.h>
#include <vm.h>

void
do_tlbie(union pte *pte, uval ptex)
{
	uval64 vsid,
	       group,
	       pi,
	       pi_high;
	uval64 virtualAddress;

	vsid = pte->bits.avpn >> 5;
	group = ptex >> 3;
	if (pte->bits.h) {
		group = ~group;
	}
	pi = (vsid ^ group) & 0x7ff;
	pi_high = (pte->bits.avpn & 0x1f) << 11;
	pi |= pi_high;
	virtualAddress = (pi << 12) | (vsid << 28);
	if (pte->bits.l) {
		virtualAddress |= (pte->bits.rpn & 1);
		tlbie_large(virtualAddress);
	} else {
		tlbie(virtualAddress);
	}

	eieio();
	tlbsync();
	ptesync();
}

void
pte_tlbie(union pte *pte, struct os *os)
{
	uval ptex = (pte - (union pte *)GET_HTAB(os)) >> LOG_PTE_SIZE;

	pte->bits.v = 0;
	ptesync();
	do_tlbie(pte, ptex);
}

void
ptes_tlbies(struct os *os, struct logical_chunk_info *lci)
{
	uval ptex;

	for (ptex = 0; ptex < os->htab.num_ptes; ptex++) {
		union pte *pte = ((union pte *)GET_HTAB(os)) + ptex;
		uval *shadow = os->htab.shadow + ptex;

		if (!pte->bits.v) {
			continue;
		}

		if (!range_contains(lci->lci_laddr, lci->lci_size, *shadow)) {
			continue;
		}
		pte->bits.v = 0;
		ptesync();
		do_tlbie(pte, ptex);
		eieio();
		tlbsync();
		ptesync();
	}
}
