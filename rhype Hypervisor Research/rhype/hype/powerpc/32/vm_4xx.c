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
#include <types.h>
#include <lpar.h>
#include <hype.h>
#include "tlb_4xx.h"
#include <os.h>
#include <mmu.h>

#define CHECK_INDEX(x)	(x >= NUM_TLBENTRIES)
#define EADDR_TO_TLBX(eaddr)	tlbsx(eaddr)

/*
 * Implement the H_REMOVE hcall() for a 32-bit OS.
 */

uval
h_remove(struct cpu *pcop, uval flags, uval tlb_id)
{
	uval tlbx = tlb_id;

	/* XXX
	 * handle flags:
	 *      H_AVPN
	 *      H_ANDCOND
	 */

	/* remove all OS TLBEs */
	if (flags & H_ALL) {
		for (tlbx = MIN_OS_TLBX; tlbx < pcop->tlb_lowest_bolted;
		     ++tlbx) {
			if (pcop->utlb[tlbx].bits.v) {
				tlbwe(tlbx, 0, 0, 0);
				pcop->utlb[tlbx].words.epnWord = 0;
			}
		}
		return H_Success;
	}

	/* otherwise remove only 1 */
	if (flags & H_EADDR) {
		if (-1 == (tlbx = EADDR_TO_TLBX(tlb_id)))
			return H_NOT_FOUND;
	} else if (CHECK_INDEX(tlb_id)) {
		return H_Parameter;
	}

	/* load up gpr3-5 with the old TLBE's words */
	tlbre(tlbx, &pcop->reg_gprs[4], &pcop->reg_gprs[5],
	      &pcop->reg_gprs[6]);

	/* now invalidate that TLBE. ignore MMUCR:STID, since V=0 */
	tlbwe(tlbx, 0, 0, 0);

	pcop->utlb[tlbx].words.epnWord = 0;

	return H_Success;
}

/*
 * Implement the H_ENTER hcall() for a 32-bit OS.
 */

uval
h_enter(struct cpu *pcop, uval flags, uval tlb_id,
	uval epnWord, uval rpnWord, uval attribWord)
{
	union tlbe localTlbe;
	uval tlbx = tlb_id;

	if (flags & H_BOLTED) {
		tlbx = -1;
		if (flags & H_EADDR) {
			tlbx = EADDR_TO_TLBX(tlb_id);
			if (tlbx != -1 && tlbx < pcop->tlb_lowest_bolted) {
				/* We're adding a bolted TLBE, replacing a previous non-bolted
				 * one. Invalidate the old non-bolted entry.
				 */
				tlbwe(tlbx, 0, 0, 0);
				tlbx = -1;
			}
		}
		if (tlbx == -1) {
			tlbx = --pcop->tlb_lowest_bolted;
			assert(pcop->tlb_lowest_bolted > 4,
			       "too many bolted TLBEs!");
		}
	} else if (flags & H_EADDR) {
		if (-1 == (tlbx = EADDR_TO_TLBX(tlb_id))) {
			/* select new index with wraparound */
			tlbx = pcop->tlb_last_used + 1;
			if (tlbx >= pcop->tlb_lowest_bolted) {
				tlbx = MIN_OS_TLBX;
			}
			pcop->tlb_last_used = tlbx;
		}
	} else if (CHECK_INDEX(tlb_id)) {
		return H_Parameter;
	}

	localTlbe.words.epnWord = epnWord;
	localTlbe.words.rpnWord = rpnWord;
	localTlbe.words.attribWord = attribWord;

	/* translate OS's Real to hypervisor's Logical */
	localTlbe.bits.rpn = RPN_R2L(pcop, localTlbe.bits.rpn);

	/* XXX
	 * validate RPN (including page size)
	 * validate attribute bits (IO vs memory)
	 * clear reserved bits
	 * handle flags:
	 *      H_ZERO_PAGE
	 *      H_ICACHE_INVALIDATE
	 *      H_ICACHE_SYNCHRONIZE
	 *      H_EXACT
	 *      H_LARGE_PAGE
	 */

	/* Record the TID so we can get later context switches right. */
	localTlbe.bits.tid = get_mmucr() & MMUCR_STID_MASK;

	/* store TLBE in struct cpu's TLB mirror */
	pcop->utlb[tlbx] = localTlbe;

	/* enter TLBE into the UTLB */
	tlbwe(tlbx, localTlbe.words.epnWord, localTlbe.words.rpnWord,
	      localTlbe.words.attribWord);

	pcop->reg_gprs[4] = tlbx;

	return H_Success;
}

/*
 * Implement the H_READ hcall() for a 32-bit OS.
 */

uval
h_read(struct cpu *pcop, uval flags, uval tlb_id)
{
	union tlbe localTlbe;
	uval tlbx = tlb_id;

	/* XXX flag H_READ_4 won't work for 3-word TLBEs... */

	if (flags & H_EADDR) {
		if (-1 == (tlbx = EADDR_TO_TLBX(tlb_id)))
			return H_NOT_FOUND;
	} else if (CHECK_INDEX(tlb_id)) {
		return H_Parameter;
	}

	/* load up gpr4-6 with the old TLBE's words */
	tlbre(tlbx, &pcop->reg_gprs[4], &pcop->reg_gprs[5],
	      &pcop->reg_gprs[6]);

	/* set the OS's MMUCR from hardware */
	pcop->reg_mmucr.stid = get_mmucr() & MMUCR_STID_MASK;

	/* untranslate RPN */
	localTlbe.words.rpnWord = pcop->reg_gprs[5];
	localTlbe.bits.rpn = RPN_L2R(pcop, localTlbe.bits.rpn);
	pcop->reg_gprs[5] = localTlbe.words.rpnWord;

	return H_Success;
}

/*
 * Implement the H_CLEAR_MOD hcall() for a 32-bit OS.
 */

uval
h_clear_mod(struct cpu *pcop, uval flags, uval ptex)
{
	return H_Function;
}

/*
 * Implement the H_CLEAR_REF hcall() for a 32-bit OS.
 */

uval
h_clear_ref(struct cpu *pcop, uval flags, uval ptex)
{
	return H_Function;
}

/*
 * Implement the H_PROTECT hcall() for a 32-bit OS.
 */

uval
h_protect(struct cpu *pcop, uval flags, uval tlb_id)
{
	union tlbe tlbe;
	uval tlbx = tlb_id;

	if (flags & H_EADDR) {
		if (-1 == (tlbx = EADDR_TO_TLBX(tlb_id)))
			return H_NOT_FOUND;
	} else if (CHECK_INDEX(tlb_id)) {
		return H_Parameter;
	}

	tlbre(tlbx, &tlbe.words.epnWord, &tlbe.words.rpnWord,
	      &tlbe.words.attribWord);

	tlbe.bits.up = 0;
	tlbe.bits.up |= !!(flags & H_UX) << 2;
	tlbe.bits.up |= !!(flags & H_UW) << 1;
	tlbe.bits.up |= !!(flags & H_UR);

	tlbe.bits.sp = 0;
	tlbe.bits.sp |= !!(flags & H_SX) << 2;
	tlbe.bits.sp |= !!(flags & H_SW) << 1;
	tlbe.bits.sp |= !!(flags & H_SR);

	tlbwe(tlbx, tlbe.words.epnWord, tlbe.words.rpnWord,
	      tlbe.words.attribWord);

	return H_Success;
}

/*
 * Implement the H_GET_TCE hcall() for a 32-bit OS.
 */

uval
h_get_tce(struct cpu *pcop)
{
	return H_Function;
}

/*
 * Implement the H_PUT_TCE hcall() for a 32-bit OS.
 */

uval
h_put_tce(struct cpu *pcop)
{
	return H_Function;
}
