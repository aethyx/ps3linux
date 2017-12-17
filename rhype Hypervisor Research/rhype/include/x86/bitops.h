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
#ifndef _X86_BITOPS_H
#define _X86_BITOPS_H

static inline uval
bit_log2(uval x)
{
	if (x == 0) {
		return 32;
	}
	__asm__("bsrl %1,%0": "=r"(x) : "r"(x));
	return x;
}

/*
 * WARNING unlike ffs(), ffz return a zero based result
 */
static inline uval
ffz(uval i)
{
	if (~i == 0) {
		return 32;
	}
	__asm__("bsfl %1,%0" : "=r"(i) : "r"(~i));
	return i;
}

#endif /* _X86_BITOPS_H_ */
