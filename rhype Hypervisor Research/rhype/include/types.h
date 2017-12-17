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

#ifndef __TYPES_H__
#define __TYPES_H__

#include <config.h>
#include <stddef.h>

#define _SIZEUVAL 8		/* size UVAL in bytes */

#ifndef __ASSEMBLY__

/* should not be here, but cannot think of a better place */
extern void *alloca(size_t);

typedef long sval;
typedef signed char sval8;
typedef short sval16;
typedef int sval32;
typedef long long sval64;

typedef unsigned long uval;
typedef unsigned char uval8;
typedef unsigned short uval16;
typedef unsigned int uval32;
typedef unsigned long long uval64;
typedef struct _uval128 {
	uval64 _uval128_hi;
	uval64 _uval128_lo;
} uval128 __attribute__ ((aligned(16)));

#define __align_16	__attribute__ ((aligned(2)))
#define __align_32	__attribute__ ((aligned(4)))
#define __align_64	__attribute__ ((aligned(8)))
#define __align_128	__attribute__ ((aligned(16)))


#endif /* ! __ASSEMBLY__ */
#endif /* ! __TYPES_H__ */
