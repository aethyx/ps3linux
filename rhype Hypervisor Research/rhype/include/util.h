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

/* This file contains all the CPP macros that are handy for everyone,
 * inclusion of this file should not effect any code anywhere */

#ifndef _UTIL_H_
#define _UTIL_H_

/* Assembler does not like ULL but we need it for C */
#ifdef __ASSEMBLY__
#define ULL(x) (x)
#define UL(x) (x)

#else

#include <types.h>

#define ULL(x) x ## ULL
#define UL(x) x ## UL


#define DECLARE(sym, val) \
	asm volatile("\n#define\t" #sym "\t%0" : : "i" (val))

/* s must be a power of 2 */
#define ALIGN_UP(a,s)	(((a) + ((s) - 1)) & (~((s) - 1)))
#define ALIGN_DOWN(a,s)	((a) & (~((s) - 1)))


#define MIN(a,b)	((a) < (b) ? (a) : (b))
#define MAX(a,b)	(((a) > (b)) ? (a) : (b))

#define PAD_TO(t, s, stype) \
	unsigned char _pad_to_ ## t[(t) - (s + sizeof (stype))]

#define DRELA(p,b) ((__typeof__ (p))((((uval)(p)) + (b))))

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

/* Given a pointer to a "member" of a struct "type", return the address
   of the struct. */
#define PARENT_OBJ(type, member, ptr)				\
	((type*)(((uval)ptr) - ((uval)&((type*)NULL)->member)))



/*
 * Returns the bit position of the smallest power of 2 that is greater
 * than n.
 */
static __inline__ uval
log2max(uval n)
{
	uval s = 1;
	uval i = 0;

	while (s < n) {
		s <<= 1;
		++i;
	}
	return i;
}
#endif /* ! __ASSEMBLY__ */


#endif /* _UTIL_H_ */
