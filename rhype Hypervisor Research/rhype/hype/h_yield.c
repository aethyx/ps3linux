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
#include <h_proto.h>
#include <hype.h>

/*
 * Yields this specific CPU, OS keeps running on other CPUs.
 * Since interrupts are disabled and this is a per-CPU operation,
 * no locking is required.
 */

sval
h_yield(struct cpu_thread *thread, uval lpid)
{
//hprintf("h_yield: Self 0x%x: lpid 0x%lx\n", thread->cpu->os->po_lpid, lpid);
	/* For now, lpid ==0 --> wait for interrupt,
	 * make ourselves un-runnable.
	 */
	thread->cpu->sched.next_lpid = 0;
	thread->preempt = CT_PREEMPT_YIELD_SELF;

	if (lpid == 0) {
		thread->cpu->sched.runnable = 0;
	} else if (lpid != (uval)H_SELF_LPID) {
		struct os *next = os_lookup(lpid);

		if (next == NULL) {
			return H_Parameter;
		}
		thread->cpu->sched.next_lpid = lpid;
		thread->preempt = CT_PREEMPT_YIELD_DIRECTED;
	}

	return (H_Success);
}
