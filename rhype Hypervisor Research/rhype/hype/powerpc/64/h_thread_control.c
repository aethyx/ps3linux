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
#include <pmm.h>
#include <cpu_control_area.h>

sval
h_thread_control(struct cpu_thread *thread, uval flags,
		 uval thread_num, uval start_addr)
{
	uval rc = H_Parameter;

	switch (flags) {
	case HTC_INIT:
		if (thread->cpu->os == ctrl_os) {
			uval rc;

			rc = cca_hard_start(thread->cpu, thread_num,
					    start_addr);
			if (rc) {
				/* this implicitly causes a yield self
				 * so the the hdecs can
				 * matchup.. maybe */
				/* but since this happens before other
				 * LPARS are up ... */
				thread->cpu->sched.next_lpid = 0;
				thread->preempt = CT_PREEMPT_YIELD_SELF;

				return H_Success;
			}
		}
	case HTC_START:
		{
			int cpu_index;
			int thread_index;
			struct cpu_thread *target_thread;
			uval icpus;

			icpus = thread->cpu->os->installed_cpus *
				THREADS_PER_CPU;

			if (thread_num >= icpus) {
				rc = H_Parameter;
				break;
			}

			if (start_addr >= CHUNK_SIZE) {
				rc = H_Parameter;
				break;
			}

			cpu_index = thread_num / THREADS_PER_CPU;
			thread_index = thread_num % THREADS_PER_CPU;

			target_thread =
				thread->cpu->os->cpu[cpu_index]->thread +
				thread_index;

			lock_acquire(&target_thread->lock);

			if (target_thread->state != CT_STATE_OFFLINE) {
				rc = H_Parameter;
			} else {
				target_thread->reg_srr0 = start_addr;
				target_thread->reg_hsrr0 = start_addr;
				target_thread->reg_srr1 =
					target_thread->cpu->os->po_boot_msr;
				target_thread->reg_hsrr1 =
					target_thread->cpu->os->po_boot_msr;
				target_thread->state = CT_STATE_EXECUTING;
			}

			lock_release(&target_thread->lock);

			break;
		}
	case HTC_STOP_SELF:
		assert(0, "TODO\n");
		break;

	case HTC_QUERY:
		assert(0, "TODO\n");
		break;
	}

	return rc;
}
