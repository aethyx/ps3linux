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

#include <h_proto.h>
#include <crq.h>
#include <debug.h>

sval
h_copy_rdma(struct cpu_thread *thread, uval length,
	    uval sliobn, uval sioba, uval dliobn, uval dioba)
{
	sval ret;

	DEBUG_OUT(DBG_CRQ, "%s: length=%ld sliobn=0x%08lx "
		  "sioba=0x%08lx dliobn=0x%08lx dioba=0x%08lx\n",
		  __func__, length, sliobn, sioba, dliobn, dioba);

	if (0 == length) {
		return H_Parameter;
	}

	ret = crq_try_copy(thread, sliobn, sioba, dliobn, dioba, length);
	return ret;
}
