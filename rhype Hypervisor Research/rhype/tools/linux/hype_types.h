/*
 * Copyright (C) 2005 Michal Ostrowski <mostrows@watson.ibm.com>, IBM Corp.
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
 * Hype definitions for user applications.
 *
 */
#ifndef HYPE_TYPES_H
#define HYPE_TYPES_H
#include <types.h>

/* We don't define __KERNEL__ in getting Linux headers, so we
 * must provide these definitions */
typedef sval8	s8;
typedef uval8	u8;
typedef sval16	s16;
typedef uval16	u16;
typedef sval32	s32;
typedef uval32	u32;
typedef sval64	s64;
typedef uval64	u64;

/* If building as ppc32 app, make sure we get types.h from asm-ppc,
 * otherwise asm-ppc64.  This ensures that the size of oh_hcall_args
 * is always the same size (since u64 is defined as unsigned long long
 * in asm-ppc, but unsigned long in asm-ppc64)
 */

#ifdef __PPC__
#ifdef __PPC64__
#define UVAL_IS_64
#include <asm-ppc64/types.h>
#else

#include <asm-ppc/types.h>
#define _PPC64_TYPES_H
#endif /* __PPC64__ */
#else
#include <asm/types.h>
#endif /* __PPC__ */

/* Linux provides an equivalent definition */
#ifdef INVALID_MEM_RANGE
#undef INVALID_MEM_RANGE
#endif /* #ifdef INVALID_MEM_RANGE */
#include <linux/openhype.h>

#endif /* HYPE_TYPES_H*/
