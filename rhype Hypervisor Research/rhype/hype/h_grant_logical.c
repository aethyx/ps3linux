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
#include <types.h>
#include <lib.h>
#include <hcall.h>
#include <hash.h>
#include <resource.h>
#include <lpar.h>
#include <objalloc.h>
#include <os.h>
#include <h_proto.h>

sval
h_grant_logical(struct cpu_thread *thread, uval flags,
		uval logical_hi, uval logical_lo, uval length,
		uval unit_address)
{
	struct os *dest_os;

	if (!(flags & (MEM_ADDR | MMIO_ADDR | INTR_SRC))) {
		return H_Parameter;
	}

	if (unit_address == (uval)H_SELF_LPID) {
		dest_os = thread->cpu->os;
	} else {
		dest_os = os_lookup(unit_address);
	}
	if (!dest_os) {
		return H_Parameter;
	}

	struct sys_resource *res;
	sval err = grant_resource(&res, thread->cpu->os, flags,
				  logical_hi, logical_lo,
				  length, dest_os);

	if (err >= 0) {
		return_arg(thread, 1, err);
		err = H_Success;
	}

	return err;
}
