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


#ifndef __IO_H_
#define __IO_H_

#ifndef __ASSEMBLY__

#include <config.h>
#include <types.h>

extern void io_out64(volatile uval64 *addr, uval64 val);
extern void io_out64BE(volatile uval64 *addr, uval64 val);
extern void io_out64LE(volatile uval64 *addr, uval64 val);
extern uval64 io_in64(const volatile uval64 *addr);
extern uval64 io_in64BE(const volatile uval64 *addr);
extern uval64 io_in64LE(const volatile uval64 *addr);

extern void io_out32(volatile uval32 *addr, uval32 val);
extern void io_out32BE(volatile uval32 *addr, uval32 val);
extern void io_out32LE(volatile uval32 *addr, uval32 val);
extern uval32 io_in32(const volatile uval32 *addr);
extern uval32 io_in32BE(const volatile uval32 *addr);
extern uval32 io_in32LE(const volatile uval32 *addr);

extern void io_out16(volatile uval16 *addr, uval16 val);
extern void io_out16BE(volatile uval16 *addr, uval16 val);
extern void io_out16LE(volatile uval16 *addr, uval16 val);
extern uval16 io_in16(const volatile uval16 *addr);
extern uval16 io_in16BE(const volatile uval16 *addr);
extern uval16 io_in16LE(const volatile uval16 *addr);

extern void io_out8(volatile uval8 *addr, uval8 val);
extern uval8 io_in8(const volatile uval8 *addr);

extern void io_out_sz(volatile void *addr, uval64 val, uval32 sz);
extern uval64 io_in_sz(const volatile void *addr, uval32 sz);

#define io_out(a, v) io_out_sz(a, v, sizeof (*(a)))
#define io_in(a) (__typeof__ (*(a)))io_in_sz(a, sizeof (*(a)))

#endif /* ! __ASSEMBLY__ */
#endif /* ! __IO_H_ */
