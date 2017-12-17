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
 * I/O functions
 *
 */
#ifndef __HYPE_X86_IO_H__
#define __HYPE_X86_IO_H__

#include <types.h>

static inline uval8
inb(uval16 addr)
{
	uval8 val;

	/* *INDENT-OFF* */
	__asm__ __volatile__ ("inb %w1,%0" : "=a" (val) : "Nd" (addr));
	/* *INDENT-ON* */

	return val;
}

static inline void
outb(uval16 addr, uval8 val)
{
	/* *INDENT-OFF* */
	__asm__ __volatile__ ( "outb %%al, %%dx" :: "d"(addr), "a"(val));
	/* *INDENT-ON* */
}

static inline uval16
inw(uval16 addr)
{
	uval16 val;

	/* *INDENT-OFF* */
	__asm__ __volatile__ ("inw %w1,%0" : "=a"(val) : "Nd"(addr));
	/* *INDENT-ON* */

	return val;
}

static inline void
outw(uval16 addr, uval16 val)
{
	/* *INDENT-OFF* */
	__asm__ __volatile__ ( "outw %%ax, %%dx" :: "d"(addr), "a"(val));
	/* *INDENT-ON* */
}

static inline void
outl(uval32 addr, uval32 val)
{
	/* *INDENT-OFF* */
	__asm__ __volatile__ ( "outl %%eax, %%dx" :: "d"(addr), "a"(val));
	/* *INDENT-ON* */
}

#include_next<io.h>

#endif /* __HYPE_X86_IO_H__ */
