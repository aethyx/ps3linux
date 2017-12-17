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
#ifndef _X86_ATOMIC_H_
#define _X86_ATOMIC_H_

static inline void
sync_after_acquire(void)
{
}

static inline void
sync_before_release(void)
{
#ifdef notyet
	__asm__ __volatile__("mfence");
#endif
}

/*
 * CAS: atomic compare and swap
 */
static inline uval32
cas_uval32(volatile uval32 *ptr, uval32 oval, uval32 nval)
{
	unsigned char tmp;

	__asm__ __volatile__ (
		"# cas_uval32:			\n"
		"	lock			\n"
		"	cmpxchgl %1,%2		\n"
		"	sete %0			\n"
		"# end cas_uval32		\n"
		: "=q" (tmp)
		: "q" (nval), "m"(*ptr), "a"(oval)
		: "memory", "cc");
	return tmp;
}

static inline uval
cas_uval64(volatile uval64 *ptr, uval64 oval, uval64 nval)
{
	/*
	 * Ugly, but we need this union to overcome C99 strict aliasing
	 * rules. This is explicitely allowed by the C99 spec.
	 */
	union cas {
		uval64 v;
		struct {
			uval32 l;
			uval32 h;
		} s;
	};
	union cas o;
	union cas n;
	unsigned char tmp;

	o.v = oval;
	n.v = nval;
	__asm__ __volatile__ (
		"# cas_uval64:			\n"
		"	lock			\n"
		"	cmpxchg8b %5		\n"
		"	sete %0			\n"
		"# end cas_uval64		\n"
		: "=q" (tmp)
		: "a" (o.s.l), "d" (o.s.h), "b" (n.s.l), "c" (n.s.h),
		  "m" (*ptr)
		: "memory", "cc");
	return tmp;
}

static inline uval
cas_uval(volatile uval *ptr, uval oval, uval nval)
{
	return cas_uval32((volatile uval32 *)ptr, (uval32)oval, (uval32)nval);
}

/* Now get more generic definitions */
#include_next <atomic.h>

#endif /* _X86_ATOMIC_H_ */
