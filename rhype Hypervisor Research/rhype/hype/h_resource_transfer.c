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
#include <h_proto.h>
#include <pmm.h>

sval
h_resource_transfer(struct cpu_thread *thread, uval type,
		    uval addr_hi, uval addr_lo, uval size, uval lpid)
{
	struct os *target_os = os_lookup(lpid);

	if (target_os == NULL) {
#ifdef DEBUG
		hprintf("PMM: lpid 0x%lx is not valid", lpid);
#endif
		return H_Parameter;
	}

	
	return resource_transfer(thread, type, addr_hi, addr_lo, size,
				 target_os);
}

