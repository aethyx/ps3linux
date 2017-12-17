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
#include <hype.h>
#include <pmm.h>
#include <os.h>
#include <h_proto.h>
#include <resource.h>
#include <boot.h>
#include <sched.h>

sval
h_destroy_partition(struct cpu_thread *thread, uval sid)
{
	uval rc = H_Parameter;
	struct os *os;

	if (sid == (uval)H_SELF_LPID) {
		os = thread->cpu->os;
	} else {
		os = os_lookup(sid);
	}

	if (thread->cpu->os == os) {
		if (os == ctrl_os) {
			boot_shutdown();
		}
#ifdef DEBUG
		hprintf("h_destroy_partition: you can't destroy yourself\n");
#endif
		return rc;
	}

	if (os) {
		int cpu;

#ifdef DEBUG
		hprintf("h_destroy_partition: destroying lpid 0x%x\n",
			os->po_lpid);
#endif

		run_event(os, LPAR_DIE);

		for (cpu = 0; cpu < MAX_CPU; ++cpu) {
			lock_acquire(&hype_per_cpu[cpu].hpc_mutex);
			os->cpu[cpu]->sched.runnable = 0;

			/* remove our scheduling reservations */
			/* XXX translate logical_cpu_num to physical! */
			locked_set_sched_params(os->cpu[cpu],
					os->cpu[cpu]->logical_cpu_num, 0, 0);

			lock_release(&hype_per_cpu[cpu].hpc_mutex);
		}

		/* Do the cleanup of resources, and an RCU deferred
		 * os_free() */
		partition_death_cleanup(os);

		rc = H_Success;

#ifdef DEBUG
		hprintf("h_destroy_partition: complete\n");
#endif
	}

	return rc;
}
