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
#include <h_proto.h>
#include <htab.h>
#include <cpu_control_area.h>
#include <thread_control_area.h>
#include <vm.h>
#include <logical.h>
#include <debug.h>
#include <tlb.h>

sval
h_vm_map(struct cpu_thread *thread, uval flags, uval pvpn, uval ptel)
{
	union ptel tlb_rpn;
	struct tlb_vpn tlb_vpn;
	union tlb_index tlb_index;
	int lpbits = 0;
	int pgsize = 12;
	uval lrpn;
	uval rrpn;
	const struct arch_mem_ops *amo;
	struct cpu_control_area *cca = get_cca();
	lock_t *tlb_lock = &cca->tlb_lock;
	sval ret;

	if (flags & ~VM_MAP_SUPPORTED_FLAGS) {
		return H_Parameter;
	}

	if (thread->cpu->os->os_partition_info[1].sfw_tlb == 0) {
		DEBUG_MT(DBG_VM_MAP, "vm_map: attempt to use h_vm_map "
			 "in non SW-TLB managed partition\n");
		return H_Parameter;
	}

	tlb_rpn.word = ptel;
	tlb_vpn.word = 0;
	tlb_index.word = 0;

	if (flags & H_VM_MAP_LARGE_PAGE) {	/* large page? */
		/* figure out the page size for the selected large page */
		uval rpn = tlb_rpn.bits.rpn;
		int lpsize = 0;

		/* set the large page bit */
		tlb_vpn.bits.l = 1;

		while (rpn & 0x1) {
			rpn = rpn >> 1;
			lpbits = ((lpbits << 1) | 0x1);
			lpsize++;
		}

		if (lpsize > 1) {
			/* only lp sizes 0 & 1 are supported on BE */
			DEBUG_MT(DBG_VM_MAP, "%s: attempt to use unsupported "
				 "lpsize %d\n", __func__, lpsize);
			return H_Parameter;
		}

		/* get correct pgshift value */
		pgsize = thread->cpu->os->large_page_shift[lpsize];
		DEBUG_MT(DBG_VM_MAP, "h_vm_map: mapping for large page size "
			 "2^%d being processed\n", pgsize);
	}
	/* get the correct logical RPN in terms of 4K pages need to
	 * mask off lp bits and unused arpn bits if this is a large
	 * page */

	lrpn = ~0ULL << (pgsize - 12);
	lrpn = tlb_rpn.bits.rpn & lrpn;

	uval laddr = lrpn << LOG_PGSIZE;
	struct logical_chunk_info *lci = laddr_to_lci(thread->cpu->os, laddr);

	if (!lci) {
		return H_Parameter;
	}

	/* translate the logical RPN to the real RPN */
	rrpn = lci_translate(lci, laddr, 1 << pgsize);

	if (rrpn == INVALID_PHYSICAL_ADDRESS) {
		DEBUG_MT(DBG_VM_MAP, "%s: given invalid LRPN: 0x%lx\n",
			 __func__, lrpn);
		ret = H_Parameter;
		goto done;
	}

	rrpn >>= LOG_PGSIZE;

	if (rrpn == 0) {
		/*
		 * if we get here and fail (0) then we must have tried
		 * to map a privilaged area
		 */
		DEBUG_MT(DBG_VM_MAP, "%s: Privileged :RPN: 0x%lx\n",
			 __func__, lrpn);
		ret = H_Privilege;
		goto done;
	}

	if ((flags & H_VM_MAP_INSERT_TRANSLATION) &&
	    (flags & H_VM_MAP_INVALIDATE_TRANSLATION)) {
		DEBUG_MT(DBG_VM_MAP, "%s: fool, you can't insert and "
			 "invalidate a translation\n", __func__);
		ret = H_Parameter;
		goto done;
	}

	if (!((flags & H_VM_MAP_INSERT_TRANSLATION) ||
	      (flags & H_VM_MAP_INVALIDATE_TRANSLATION))) {
		DEBUG_MT(DBG_VM_MAP, "%s: huh, why are you asking "
			 "us to do nothing?\n", __func__);
		ret = H_Parameter;
		goto done;
	}

	amo = lci->lci_arch_ops;

	/* check protection bits are appropriate (IO vs memory) */
	if (amo->amo_check_WIMG &&
	    !(*(amo->amo_check_WIMG)) (lci, laddr, (union ptel)tlb_rpn.word)) {
		ret = H_Parameter;
		goto done;
	}

	if (flags & H_VM_MAP_ZERO_PAGE) {
		clear_page(rrpn, pgsize);
	}

	if (flags & H_VM_MAP_ICACHE_INVALIDATE) {
		icache_invalidate_page(rrpn, pgsize);
	}

	if (flags & H_VM_MAP_ICACHE_SYNCRONIZE) {
		icache_synchronize_page(rrpn, pgsize);
	}

	if (flags & H_VM_MAP_INSERT_TRANSLATION) {
		uval vpn_low = 0;
		uval vpn_high = 0;
		int ccindex = 0;

		/* insert a translation */
		DEBUG_MT(DBG_VM_MAP, "%s: should map RRPN 0x%lx here\n",
			 __func__, rrpn);

		/* swap to tho RRPN */
		tlb_rpn.bits.rpn = rrpn | lpbits;

		/* set the valid bit */
		tlb_vpn.bits.v = 1;

		/* get the vpn bits into the right places */
		vpn_low = pvpn & 0x7FF;
		vpn_high = pvpn >> 11;
		vpn_high |= ((flags & 0xF) << 53);

		tlb_vpn.bits.avpn = vpn_high;
		tlb_index.bits.lvpn = vpn_low;

		/* find the congruance class - we can just use the
		 * pvpn for this since it has all the bits we ever
		 * need */
		switch (pgsize) {
		case 12:
			ccindex = (((pvpn >> 8) & 0xF0) ^ ((pvpn) & 0xFF));
			break;
		case 16:
			ccindex = (((pvpn >> 8) & 0xF0) ^
				   ((pvpn >> 4) & 0xFF));
			break;
		case 20:
			ccindex = ((pvpn >> 8) & 0xFF);
			break;
		case 24:
			ccindex = ((pvpn >> 12) & 0xFF);
			break;
		default:
			DEBUG_MT(DBG_VM_MAP, "%s: unsupported page size\n",
				 __func__);
			break;
		}

		ccindex = ccindex << 4;

		lock_acquire(tlb_lock);

		cca->tlb_index_randomizer += 1;
		if (cca->tlb_index_randomizer == 4) {
			cca->tlb_index_randomizer = 0;
		}

		DEBUG_MT(DBG_VM_MAP, "%s: vpn 0x%llx rpn 0x%llx, "
			 "ccindex 0x%x entry %d\n",
			 __func__, tlb_vpn.word, tlb_rpn.word,
			 ccindex, cca->tlb_index_randomizer);

		tlb_index.bits.index = ccindex |
			(1 << cca->tlb_index_randomizer);

		/* insert the entry into the TLB */
		mttlb_index(tlb_index.word);
		mttlb_rpn(tlb_rpn.word);
		mttlb_vpn(tlb_vpn.word);

		lock_release(tlb_lock);

		/* we're done! */
	} else {
		uval64 virtualAddress = pvpn;

		if (tlb_vpn.bits.l) {
			virtualAddress |= (tlb_rpn.bits.rpn & 1);
			tlbie_large(virtualAddress);
		} else {
			tlbie(virtualAddress);
		}
	}
	DEBUG_MT(DBG_VM_MAP, "%s: leave\n", __func__);
	ret = H_Success;
/* *INDENT-OFF */
      done:
/* *INDENT-ON */
	return ret;
}
