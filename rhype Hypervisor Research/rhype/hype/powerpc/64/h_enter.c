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
#include <vm.h>
#include <logical.h>

sval
h_enter(struct cpu_thread *thread, uval flags, uval ptex, uval vsidWord,
	uval rpnWord)
{
	union pte localPte;
	union pte *chosen_pte;
	union pte *cur_htab_base = (union pte *)GET_HTAB(thread->cpu->os);
	int lpbits = 0;
	int pgsize = LOG_PGSIZE;
	uval chosen_index;
	uval limit = 0;		/* how many PTEs to examine in the PTEG */
	uval lrpn;
	uval rrpn;
	sval ret;
	const struct arch_mem_ops *amo;

	/* use local HPTE to avoid manual shifting & masking */
	localPte.words.vsidWord = vsidWord;
	localPte.words.rpnWord = rpnWord;

#if 0
	if ((vsidWord >> 12) != 0x6a99b4) {
		hprintf("%s: vsid 0x%lx\n", __func__, vsidWord);
	}
#endif

	if (thread->cpu->os->os_partition_info[1].sfw_tlb == 1) {
#ifdef SWTLB
		hprintf("%s: attempted in a SW-TLB managed partition\n",
			__func__);
		return H_Parameter;
#else  /* SWTLB */
		hprintf("%s: sw tlb switched on with no sw tlb available",
			__func__);
#endif /* SWTLB */
	}

	if (CHECK_UNSUP(flags)) {
#ifdef DEBUG
		hprintf("%s: usuported flags: 0x%lx\n", __func__, flags);
#endif
		return H_Parameter;
	}

	if (localPte.bits.l) {	/* large page? */
		/* figure out the page size for the selected large page */
		uval rpn = localPte.bits.rpn;
		int lpsize = 0;

		while (rpn & 0x1) {
			rpn = rpn >> 1;
			lpbits = ((lpbits << 1) | 0x1);
			lpsize++;
		}

		if (lpsize > 1) {
			/* only lp sizes 0 & 1 are supported on BE */
			hprintf("%s: attempt to use unsupported lpsize %d\n",
				__func__, lpsize);
			return H_Parameter;
		}

		/* get correct pgshift value */
		pgsize = thread->cpu->os->large_page_shift[lpsize];
	}

	/* get the correct logical RPN in terms of 4K pages
	 * need to mask off lp bits and unused arpn bits if this is a
	 * large page */

	lrpn = ~0ULL << (pgsize - 12);
	lrpn = localPte.bits.rpn & lrpn;

	uval laddr = lrpn << LOG_PGSIZE;
	struct logical_chunk_info *lci = laddr_to_lci(thread->cpu->os, laddr);

	if (lci == NULL) {
#ifdef DEBUG
		hprintf("%s: no lci found for %lx\n", __func__, laddr);
#endif
		return H_Parameter;
	}

	/* translate the logical RPN to the real RPN */
	rrpn = lci_translate(lci, laddr, 1 << pgsize);

	if (rrpn == INVALID_PHYSICAL_ADDRESS) {
#ifdef DEBUG
		hprintf("%s: given invalid LRPN: 0x%lx\n", __func__, lrpn);
#endif
		ret = H_Parameter;
		goto done;
	}

	rrpn >>= LOG_PGSIZE;

	if (rrpn == 0) {
		/* if we get here and fail (0) then we must
		 * have tried to map a privilaged area */
#ifdef DEBUG
		hprintf("%s: Privileged :RPN: 0x%lx\n", __func__, lrpn);
#endif
		ret = H_Privilege;
		goto done;
	}
#ifdef HE_DEBUG
	hprintf("%s: lrpn=0x%lx rrpn=0x%lx slot=0x%lx\n",
		__func__, lrpn, rrpn, ptex);
#endif

	/* fixup the RPN field of our local PTE copy */
	localPte.bits.rpn = rrpn | lpbits;

	if (check_index(thread->cpu->os, ptex)) {
#ifdef DEBUG
		hprintf("%s: bad index (0x%lx)\n", __func__, thread->cpu->os->htab.num_ptes);
#endif
		ret = H_Parameter;
		goto done;
	}

	assert(lci, "Should have found an lci, "
	       "since translation has already succeeded\n");

	amo = lci->lci_arch_ops;

	/* check protection bits are appropriate (IO vs memory) */
	if (amo->amo_check_WIMG &&
	    !(*(amo->amo_check_WIMG)) (lci, laddr,
				       (union ptel)localPte.words.rpnWord)) {
#if 0
		ret = H_Parameter;
		goto done;
#else
		;
#endif
	}

	/* clear reserved bits in high word */
	localPte.bits.lock = 0x0;
	localPte.bits.res = 0x0;
	/* localPte.bits.res1 = 0x0; XXX LPAR doc doesn't say to zero this */
	/* clear reserved bits in low word */
	localPte.bits.pp0 = 0x0;
	localPte.bits.ts = 0x0;
	localPte.bits.res2 = 0x0;

	if (!(flags & H_EXACT)) {
		/* PTEG (not specific PTE); clear 3 lowest bits */
		ptex &= ~0x7;
		limit = 7;
	}

	/*
	 * 18.5.4.1.2 demands that each of these page data manipulations
	 * be done prior to the pte insertion.
	 */
	if (flags & H_ZERO_PAGE) {
		clear_page(rrpn, pgsize);
	}

	if (flags & H_ICACHE_INVALIDATE) {
		icache_invalidate_page(rrpn, pgsize);
	}

	if (flags & H_ICACHE_SYNCHRONIZE) {
		icache_synchronize_page(rrpn, pgsize);
	}

	for (chosen_index = ptex; chosen_index <= ptex + limit; chosen_index++) {
		chosen_pte = &cur_htab_base[chosen_index];

		if (!chosen_pte->bits.v) {
			/* got it */

			if (amo->amo_sav_ptep &&
			    amo->amo_sav_ptep(lci, laddr,
					      rrpn, chosen_pte) == 0) {
#ifdef DEBUG
				hprintf("%s: no ptep, rrpn %lx\n",
					__func__, rrpn);
#endif
				ret = H_UNAVAIL;
				goto done;
			}
			pte_insert(&thread->cpu->os->htab,
				   chosen_index, localPte.words.vsidWord,
				   localPte.words.rpnWord, lrpn);

			atomic_add32(&lci->lci_ptecount, 1);
			/* return chosen PTE index */
			thread->reg_gprs[4] = chosen_index;
			ret = H_Success;
			goto done2;
		}
	}

	/* If the PTEG is full then no additional values are returned. */
	ret = H_PTEG_Full;	/* no invalid PTEs in specified PTEG! */
	goto done;
/* *INDENT-OFF* */
done2:
/* *INDENT-ON* */
	return ret;
/* *INDENT-OFF* */
done:
/* *INDENT-ON* */
	hprintf("h_enter error: %ld\n",ret);
	goto done2;
}
