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
 * slb routines
 *
 */

#include <config.h>
#include <types.h>
#include <lib.h>
#include <mmu.h>


/* does everything but set the index in the vsid */
void
slb_calc(uval ea, uval8 is_lp, uval8 lp_selection, uval *vsid, uval *esid)
{
	uval vsid_val;
	uval esid_val;

	esid_val = get_esid(ea);
	*esid = esid_val | (1UL << (63 - 36)); /* valid */

	*vsid = 0;
	if (is_lp) {
#ifdef DEBUG
		hprintf("slb_insert for large page - selection = %d\n",
			lp_selection);
#endif
		*vsid |= 0x1UL << (63 - 55);      /* large page   */
		*vsid |= lp_selection << (63-59); /* lp selection */
	}

	vsid_val = get_vsid(ea);
	*vsid |= vsid_val  << (63 - 51);
	*vsid |= 0x1UL << (63 - 53); /* Kp = 1 */
}


int
slb_insert(uval ea, uval8 is_lp, uval8 lp_selection)
{
	int i;
	int spot = 64;
	uval vsid;
	uval esid;
	slb_calc(ea, is_lp, lp_selection, &vsid, &esid);
	hprintf("Setting slb entry: %lx %lx\n", vsid, esid);
	for (i = 0; i < 64; i++) {
		/* The first one (SLB[0] always belongs to the kernel
		 * and slbia will not invalidate that one
		 */
		uval esid_ent;
		uval vsid_ent;
		__asm__ __volatile__("\n\t"
				     "isync		\n\t"
				     "slbmfee  %0,%2	\n\t"
				     "slbmfev  %1,%2	\n\t"
				     "isync		\n\t"
				     : "=r" (esid_ent),
				       "=r" (vsid_ent)
				     : "r" (i)
				     : "memory");

		/* check if not valid */
		/* choose lowest empty spot */
		if (!(esid_ent & (1UL << (63 - 36))) && spot > i) {
			spot = i;
		}
		if (esid_ent == esid) {
			spot = i;
			break;
		}

	}

	if (spot < 64) {
#ifdef DEBUG
		hprintf("Move to slb entry %d, slbmte 0x%lx, 0x%lx\n",
			spot, vsid, esid);
#endif

		/* only 12 bits */
		esid |= spot & ((0x1UL << (63 - 52 + 1)) - 1);

		__asm__ __volatile__("\n\t"
				     "sync		\n\t"
				     "isync		\n\t"
				     "slbmte %0,%1	\n\t"
				     "isync		\n\t"
				     "sync		\n"
				     :
				     : "r"(vsid), "r"(esid)
				     : "memory");
		return spot;
	}

	assert(0, "slb_insert: we don't invalidate slbe's yet\n");
	for (;;);
	/* when you do invalidate and replace make sure you leave
	 * SLB[0] alone */
}
