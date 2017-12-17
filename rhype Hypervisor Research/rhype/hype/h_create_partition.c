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
#include <os.h>
#include <resource.h>
#include <hype.h>
#include <h_proto.h>
#include <hcall.h>

sval
h_create_partition(struct cpu_thread *thread, uval donated_laddr,
		   uval laddr_size, uval pinfo_offset)
{
	struct os *os = os_create();
	uval rc;

	if (os == NULL) {
		return H_Parameter;
	}

	rc = resource_transfer(thread, MEM_ADDR,
			       0, donated_laddr, laddr_size, os);
	if (rc == H_Success) {
		/* now that we have memory, we can arch_os_init() */
		rc = arch_os_init(os, pinfo_offset);
	}

	if (rc == H_Success) {
		return_arg(thread, 1, os->po_lpid);
		return H_Success;
	}

	os_free(os);
	return H_Parameter;
}
