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

#include <cpu.h>

/* tlb_switch()
 * Starting with #3, copy TLBEs from new_pcop->utlb into the hw UTLB.
 * No need to save the old TLBEs off, that is done in h_enter().
 *
 *	new_pcop - pointer to per_cpu_os being switched in
 */
void
tlb_switch(struct cpu *new_cpu new_pcop)
{
	int i;

	/* write TLBEs 3 to NUM_TLBENTRIES to the hw UTLB */
	for (i = 3; i < NUM_TLBENTRIES; i++) {
		/* zero STID field */
		uval old_mmucr = get_mmucr() & ~MMUCR_STID_MASK;

		/* first write the new STID to hw MMUCR */
		set_mmucr(old_mmucr | new_pcop->utlb[i].bits.tid);

		/* now write the new TLBE, overwriting whatever was there */
		tlbwe(i, new_pcop->utlb[i].words.epnWord,
		      new_pcop->utlb[i].words.rpnWord,
		      new_pcop->utlb[i].words.attribWord);
	}
}
