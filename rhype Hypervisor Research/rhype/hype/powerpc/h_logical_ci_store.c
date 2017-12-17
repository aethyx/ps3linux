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

/*
 * TODO: implement these checks
 */

/* check if address is within the range for this partition */
#define CHECK_ADDRESS(os, addr)	1

/* check if address is mapped as cache inhibited */
#define CACHE_INHIBITED(os, addr)	1

/* check if address is mapped as cacheable */
#define CACHEABLE(os, addr)		1

sval
h_logical_ci_store(struct cpu_thread *thread, uval size, uval addr, uval value)
{
	uval8 *ptr8;
	uval16 *ptr16;
	uval32 *ptr32;
	uval *ptr;

	addr = logical_to_physical_address(thread->cpu->os, addr, size);

	if (addr == INVALID_PHYSICAL_ADDRESS) {
		return H_Parameter;
	}

	if (!CHECK_ADDRESS(thread->cpu->os, addr)) {
		return H_Parameter;
	}

	if (!CACHE_INHIBITED(thread->cpu->os, addr)) {
		return H_Parameter;
	}

	switch (size) {
	case 1:		/* 8-bit  */
		ptr8 = (uval8 *)addr;
		*ptr8 = (uval8)value;
		return H_Success;
	case 2:		/* 16-bit */
		if (addr & 0x1) {
			break;
		}
		ptr16 = (uval16 *)addr;
		*ptr16 = (uval16)value;
		return H_Success;

	case 3:		/* 32-bit */
		if (addr & 0x3) {
			break;
		}
		ptr32 = (uval32 *)addr;
		*ptr32 = (uval32)value;
		return H_Success;

	case 4:		/* 64-bit */
		if ((addr & (sizeof (uval) - 1)) != 0) {
			break;
		}
		ptr = (uval *)addr;
		*ptr = (uval)value;
		return H_Success;
	}

	return H_Parameter;
}
