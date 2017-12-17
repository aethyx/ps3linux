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

#ifndef _32_ATOMIC_H
#define _32_ATOMIC_H

#include <config.h>
#include <lib.h>

/* prototype, definition below */
static inline uval cas_uval(volatile uval *ptr,	uval oval, uval nval);

#include_next <atomic.h>

static inline uval
cas_uval64(volatile uval64 *ptr, uval64 oval, uval64 nval)
{
	/* not sure how to do this yet */
	ptr = ptr;
	oval = oval;
	nval = nval;
	assert(0, "no cas for 64 bit quantity yet\n");
	return 0;
}

static inline uval
cas_uval(volatile uval *ptr, uval oval, uval nval)
{
	return cas_uval32((volatile uval32 *)ptr, (uval32)oval, (uval32)nval);
}

#endif /* ! _32_ATOMIC_H */
