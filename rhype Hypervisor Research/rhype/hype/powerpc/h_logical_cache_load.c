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

#include <config.h>
#include <lpar.h>
#include <hype.h>
#include <mmu.h>
#include <pmm.h>
#include <h_proto.h>

sval
h_logical_cache_load(struct cpu_thread *thread, uval size, uval addr)
{
	uval val;

	/* is this in onr of our chunks? */
	addr = logical_to_physical_address(thread->cpu->os, addr, size);

	/*
	 * RPA states: 
	 *   The Hypervisor checks that the address is within the
	 *   range of addresses valid for the partition and mapped as
	 *   cacheable.
	 *
	 * All "Chunks" are chacheable so there is nothing to do
	 */
	switch (size) {
	default:
		return H_Parameter;

	case 1:		/* 8-bit  */
		val = *((uval8 *)addr);
		break;
	case 2:		/* 16-bit */
		if ((addr & 0x1) != 0) {
			return H_Parameter;
		}
		val = *((uval16 *)addr);
		break;
	case 4:		/* 32-bit */
		if ((addr & 0x3) != 0) {
			return H_Parameter;
		}
		val = *((uval32 *)addr);
		break;
	case 8:		/* 64-bit */
		/* on 32 bit systems we treat this as pointer size */
		if ((addr & (sizeof (uval) - 1)) != 0) {
			return H_Parameter;
		}
		val = *((uval *)addr);
		break;
	}
	return_arg(thread, 1, val);

	return H_Success;
}
