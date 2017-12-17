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
#include <sched.h>
#include <hype.h>

/*
 * h_set_sched_params(per_cpu_os *pcop, uval lpid,
 *			uval cpu, uval required, uval desired)
 *
 * Temporary interface for setting scheduling parameters.
 *
 * required/desired are bitmaps that specify which slots the calling
 * OS wants.  Bits in "required" represent scheduling slots that must
 * be assignable to this OS (and locked down).  The desired bit-map
 * represents scheduling slots that may be set/unset, without guarantees at
 * any time by HV.  Thus fullfilling "desired" requests has no bearing
 * on the success/failure of this call.
 *
 * On return, r4 contains a bitmap identifying the locked-down slots
 * (which cannot be yielded to satisfy set_sched_params() calls of
 * other partitions).  r5 contains a bitmap representing all in-use
 * scheduling slots.  The caller can use this information to try again
 * if an error has occurred. r6 contains the bitmask actually assigned
 * (a rotation of "required").
 *
 * The return value, if positive (-> success) identifies the left-wise
 * rotation required of the input parameters to have fulfilled the request.
 *
 * FIXME: Should have a mechanism to restrict rights to calls this function
 *	  to the controlling OS only.
 *
 * FIXME: Re-implement using standard LPAR interfaces, if appropriate.
 *
 * Examples of usage in "test_sched.c".
 */
sval
h_set_sched_params(struct cpu_thread *thread, uval lpid,
		   uval phys_cpu_num, uval required, uval desired)
{
	/* The real per_cpu_os to operate on is specified by the cpu arg */
	uval err = H_Success;
	struct os *target_os = os_lookup(lpid);
	struct cpu *target_cpu;
	struct hype_per_cpu_s *hpc = &hype_per_cpu[phys_cpu_num];

	/* Bounds/validity checks on lpid and cpu */
	if ((!target_os && (lpid != (uval)H_SELF_LPID)) ||
	    (phys_cpu_num > MAX_CPU && phys_cpu_num != THIS_CPU)) {
		err = H_Parameter;
		goto bad_os;
	}

	if (!target_os) {
		target_os = thread->cpu->os;
	}

	write_lock_acquire(&target_os->po_mutex);

	if (phys_cpu_num == THIS_CPU) {
		/* TODO - fixme WHAT? */

		phys_cpu_num = thread->cpu->logical_cpu_num;

		/* update our place holder */
		hpc = &hype_per_cpu[phys_cpu_num];
	}

	/* TODO - fixme WHAT? */
	target_cpu = target_os->cpu[phys_cpu_num];

	if (!target_cpu) {
		err = H_Parameter;
		goto bad_cpu;
	}

	lock_acquire(&hpc->hpc_mutex);

	err = locked_set_sched_params(target_cpu, phys_cpu_num,
				      required, desired);

	/* Provide current setting to OS, so it can compensate */
	return_arg(thread, 1, hpc->hpc_sched.locked_slots);
	return_arg(thread, 2, hpc->hpc_sched.used_slots);
	return_arg(thread, 3, target_cpu->sched.required);

	lock_release(&hpc->hpc_mutex);
/* *INDENT-OFF* */
bad_cpu:
/* *INDENT-ON* */

	write_lock_release(&target_os->po_mutex);
/* *INDENT-OFF* */
bad_os:
/* *INDENT-ON* */
	return err;
}
