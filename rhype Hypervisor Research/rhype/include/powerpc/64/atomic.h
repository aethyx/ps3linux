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
 * place for atmic operations and simple locks.
 */

#ifndef _64_ATOMIC_H
#define _64_ATOMIC_H

/* prototype, definition below */
static inline uval cas_uval64(volatile uval64 *ptr, uval64 oval, uval64 nval);

static inline uval
cas_uval(volatile uval *ptr, uval oval, uval nval)
{
	return cas_uval64((volatile uval64 *)ptr, (uval64)oval, (uval64)nval);
}

#include_next <atomic.h>

static inline uval
cas_uval64(volatile uval64 *ptr, uval64 oval, uval64 nval)
{
	uval tmp;

	sync_before_release();

	__asm__ ("\n"
		 "# cas_uval64						\n"
		 "1:	ldarx	%1,0,%4	# tmp = (*ptr)	[linked]	\n"
		 "	cmpld	%1,%2	# if (tmp != oval)		\n"
		 "	bne-	2f	#     goto failure		\n"
		 "	stdcx.	%3,0,%4	# (*ptr) = nval	[conditional]	\n"
		 "	bne-	1b	# if (store failed) retry	\n"
		 "	li	%1,1	# tmp = SUCCESS			\n"
		 "	b	$+8	# goto end			\n"
		 "2:	li	%1,0	# tmp = FAILURE			\n"
		 "# end cas_uval64					\n"
		 : "=m" (*(volatile char *)ptr), "=&r" (tmp)
		 : "r" (oval), "r" (nval), "r" (ptr), "m" (*(volatile char *)ptr)
		 : "cc"
		);
	sync_after_acquire();

	return tmp;
}

#endif /* ! _64_ATOMIC_H */
