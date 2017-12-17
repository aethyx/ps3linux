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

#include <lib.h>
#include <mmu.h>
#include <os.h>
#include <pmm.h>
#include <ipc.h>
#include <hype.h>
#include <h_proto.h>
#include <aipc.h>
#include <xirr.h>

sval
h_create_msgq(struct cpu_thread *thread, uval lbase, uval size, uval flags)
{
	sval ret;

	ret = aipc_create(thread, lbase, size, flags);

	if (ret >= 0) {
		xirr_t x = xirr_encode(ret, XIRR_CLASS_AIPC);

		return_arg(thread, 1, x);
		return H_Success;
	}
	return ret;
}
