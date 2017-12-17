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

sval
h_send_async(struct cpu_thread *thread, uval dest,
	     uval arg1, uval arg2, uval arg3, uval arg4)
{
	struct os *destos = os_lookup(dest);

	if (!destos) {
		return H_Parameter;
	}

	struct cpu_thread *thr = find_colocated_thread(thread, destos);

	if (is_colocated(thr, thread) == THREAD_MATCH) {
		thread->preempt = CT_PREEMPT_YIELD_SELF;
	}

	return ipc_send_async(thr, thread->cpu->os->po_lpid,
			      arg1, arg2, arg3, arg4);
}
