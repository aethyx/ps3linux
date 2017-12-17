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
h_real_to_logical(struct cpu_thread *thread, uval raddr)
{
	uval virt_addr;

	/* Check to see if raddr belongs to partition's assigned
	 * storage */
	/* not done - Sonny */

	virt_addr = logical_to_physical_address(thread->cpu->os, raddr, 0);

	if (virt_addr == INVALID_PHYSICAL_ADDRESS) {
		return H_Parameter;
	}

	/* Check to see if raddr belongs to IO area - return ~0UL in
	 * R4 if so */

	return_arg(thread, 1, virt_addr);
	return (H_Success);
}
