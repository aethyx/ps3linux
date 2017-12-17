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
 * Description: 44x TLB entry structure
 *
 */
#ifndef _TLBE_44x_H
#define _TLBE_44x_H

#include <config.h>

#define NUM_TLBENTRIES  64	/* number of TLB entries in 440GP UTLB */

union tlbe {
	struct TLBwords {
		uval32 epnWord;
		uval32 rpnWord;
		uval32 attribWord;
	} words;
	struct TLBbits {
		/* epnWord */
		/* *INDENT-OFF* */
		uval32 epn:   22;
		uval32 v:     1;
		uval32 ts:    1;
		uval32 size:  4;
		uval32 res1:  4;

		/* rpnWord */
		uval32 rpn:   22;
		uval32 res2:  6;
		uval32 erpn:  4;

		/* attribWord */
		uval32 res3:  16;
		uval32 u0:    1;
		uval32 u1:    1;
		uval32 u2:    1;
		uval32 u3:    1;
		uval32 wimg:  4;
		uval32 res4:  1;
		uval32 e:     1;
		uval32 up:    3; /* UX, UW, UR */
		uval32 sp:    3; /* SX, SW, SR */

		/* NOT in TLB words 0-2 but part of a "TLB entry" */
		uval32 tid:   8; /* from MMUCR:STID */

		/* *INDENT-ON* */
	} bits;
};

#endif
