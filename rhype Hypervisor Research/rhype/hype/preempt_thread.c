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
#include <thread_control_area.h>
#include <timer.h>

/*
 * Check for OS preemption.  All registers must have been saved
 * into the per_cpu_os structure, which is passed in.  This function
 * returns a pointer to the per_cpu_os structure of the new OS to
 * be switched to.
 *
 * No gang scheduling as yet.
 */
struct cpu_thread *
preempt_thread(struct cpu_thread *thread, uval timer __attribute__ ((unused)))
{
	short cpu = thread->cpu->logical_cpu_num;
	struct cpu_thread *next = NULL;
	uval locked = 0;

#ifdef DEBUG_PREEMPT
	hprintf("preempt_OS(thread lpid 0x%x), preempt ",
		thread->cpu->os->po_lpid);
	if (thread->preempt & CT_PREEMPT_RESCHEDULE)
		hprintf("RESCHEDULE ");
	if (thread->preempt & CT_PREEMPT_YIELD_SELF)
		hprintf("SELF ");
	if (thread->preempt & CT_PREEMPT_YIELD_DIRECTED)
		hprintf("RESCHEDULE_DIRECTED ");
	if (thread->preempt & CT_PREEMPT_CEDE)
		hprintf("CEDE ");
	hprintf("(0x%x)\n", thread->preempt);
#endif

	switch (thread->preempt) {
	case CT_PREEMPT_RESCHEDULE:
	case CT_PREEMPT_YIELD_SELF:
	case CT_PREEMPT_YIELD_DIRECTED:
		/* lock down the list of runnable OSes for this CPU */
		lock_acquire(&hype_per_cpu[cpu].hpc_mutex);
		locked = 1;

		/* Go find the next OS to run. */
		next = sched_next_os(thread->cpu)->thread;
		if (next == NULL) {
			/* let the runlist change now */
			lock_release(&hype_per_cpu[cpu].hpc_mutex);
			/* assembly/exit/restore code knows this means
			 * "idle" */
			return NULL;
		}

		if (next != thread) {
			arch_preempt_os(thread, next);
			update_ticks(next);
		}
		break;

	case CT_PREEMPT_CEDE:
		/* deliver any pending interrupts */
		next = thread;
		break;

	default:
		assert(0, "unknown preempt_OS directive");
	}

	/* We have yielded */
	thread->preempt = 0;

	/* We're about to start checking for deliverable exceptions */
	clear_exception_state(next);

	/* Deliver any pending interrupts */
	check_intr_delivery(next);

	/* Let the runlist change now */
	if (locked) {
		lock_release(&hype_per_cpu[cpu].hpc_mutex);
	}

	/* Switch to the new OS */
	return next;
}
