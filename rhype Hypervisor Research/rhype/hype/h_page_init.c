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
#include <hype.h>
#include <pmm.h>
#include <mmu.h>
#include <h_proto.h>

sval
h_page_init(struct cpu_thread *thread, uval flags,
	    uval destination, uval source)
{
	int large = 0;
	uval dst_rpn;
	uval src_rpn;

	destination =
		logical_to_physical_address(thread->cpu->os, destination,
					    PGSIZE);
	source = logical_to_physical_address(thread->cpu->os, source, PGSIZE);

	if ((destination == INVALID_PHYSICAL_ADDRESS) ||
	    (source == INVALID_PHYSICAL_ADDRESS)) {
		return H_Parameter;
	}

	dst_rpn = destination >> LOG_PGSIZE;
	src_rpn = source >> LOG_PGSIZE;

	/* ASSERT(address is within OS bounds)? */
	if ((destination & ~PGMASK) || (source & ~PGMASK)) {
		return H_Parameter;
	}

	if (flags & H_ZERO_PAGE) {
		clear_page(dst_rpn, large);
	}

	if (flags & H_COPY_PAGE) {
		memcpy((void *)destination, (void *)source, PGSIZE);
	}

	if (flags & H_ICACHE_INVALIDATE) {
		icache_invalidate_page(dst_rpn, large);
	}

	if (flags & H_ICACHE_SYNCHRONIZE) {
		icache_synchronize_page(dst_rpn, large);
	}

	return H_Success;
}
