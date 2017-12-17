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
#include <pmm.h>
#include <sched.h>

/*
 * Implement the H_START hcall() to start up a newly-created OS.
 * The OS's four parameters are loaded from the arguments.
 */
sval
h_start(struct cpu_thread *thread __attribute__ ((unused)),
	int lpid, uval pc, uval reg2 /* toc */ ,
	uval reg3, uval reg4, uval reg5, uval reg6, uval reg7)
{
	struct os *os = os_lookup(lpid);
	struct cpu_thread *thread0;

	if (os == NULL) {
		return (H_Parameter);
	}

	if (os->po_state != PO_STATE_CREATED) {
		return (H_Parameter);
	}

	thread0 = os->cpu[0]->thread + 0;

	/* must have already been given some scheduling bits */
	if (thread0->cpu->sched.required == 0) {
		hprintf("no sched bits\n");
		return (H_UNAVAIL);
	}

	/* call arch setup */
	arch_init_thread(os, thread0, pc, reg2, reg3, reg4, reg5, reg6, reg7);

	logical_fill_partition_info(os);

	/* now we're mapped, so we're runnable */
	os->po_state = PO_STATE_RUNNABLE;

	/*
	 * Link into CPU 0's runlist.  Other CPUs will be activated
	 * via a TBD hcall().
	 */
	run_partition(thread0);

	return (H_Success);
}
