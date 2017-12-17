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
 * $Id$
 */
/*
 *
 * Basic I/O primitives for the UART code
 *
 */
#include <config.h>
#include <types.h>
#include <io.h>

void
io_out8(volatile uval8 *addr, uval8 val)
{
	outb((uval32)addr, val);
}

uval8
io_in8(const volatile uval8 *addr)
{
	return inb((uval32)addr);
}
