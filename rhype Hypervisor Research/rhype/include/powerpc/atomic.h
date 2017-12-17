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

#ifndef _ATOMIC_H
#define _ATOMIC_H

#include <config.h>

static inline void
sync_after_acquire(void)
{
	__asm__ __volatile__ ("isync" : : : "memory");
}

static inline void
sync_before_release(void)
{
	__asm__ __volatile__ ("sync" : : : "memory");
}

/*
 * CAS : Compare and Store 32bits.. works for everyone
 *
 * NOTE:  The ptr parameters to these routines are cast to character pointers
 *        in order to prevent any strict-aliasing optimizations the compiler
 *        might otherwise attempt.
 */
static inline uval32
cas_uval32(volatile uval32 *ptr, uval32 oval, uval32 nval)
{
	uval32 tmp;

	sync_before_release();
	__asm__ ("\n"
		 "# cas_uval32						\n"
		 "1:	lwarx	%1,0,%4	# tmp = (*ptr)	[linked]	\n"
		 "	cmplw	%1,%2	# if (tmp != oval)		\n"
		 "	bne-	2f	#     goto failure		\n"
		 "	stwcx.	%3,0,%4	# (*ptr) = nval	[conditional]	\n"
		 "	bne-	1b	# if (store failed) retry	\n"
		 "	li	%1,1	# tmp = SUCCESS			\n"
		 "	b	$+8	# goto end			\n"
		 "2:	li	%1,0	# tmp = FAILURE			\n"
		 "# end cas_uval32					\n"
	: "=m" (*(volatile char *)ptr), "=&r" (tmp)
	: "r" (oval), "r" (nval), "r" (ptr), "m" (*(volatile char*)ptr)
	: "cc"
	);
	sync_after_acquire();

	return tmp;
}

/* Now get more generic definitions */
#include_next <atomic.h>

#endif /* ! _ATOMIC_H */
