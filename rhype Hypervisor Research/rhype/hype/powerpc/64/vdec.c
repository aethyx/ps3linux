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
#include <types.h>
#include <cpu_thread.h>
#include <pmm.h>
#include <vregs.h>
#include <xh.h>
#include <bitops.h>
#include <hype_control_area.h>
#include <thread_control_area.h>
#include <objalloc.h>
#include <vdec.h>

inline void
sync_from_dec(void)
{
	struct thread_control_area *tca = get_tca();
	uval diff;
	if (tca->vstate & VSTATE_ACTIVE_DEC) {
		diff = tca->active_thread->vdec - mfdec();
	} else {
		diff = tca->hdec - mfdec();
	}
//	hprintf("%s: %x %x %x\n", __func__, tca->hdec, tca->vregs->v_dec, tca->active_thread->vdec);

	tca->hdec -= diff;
	if (tca->active_thread->vdec >= 0) {
		tca->active_thread->vdec -= diff;
		tca->vregs->v_dec = tca->active_thread->vdec;
		if (tca->active_thread->vdec < 0) {
			tca->active_thread->vdec = 0;
			tca->vregs->v_dec = 0;
		}
	}


}


inline void
sync_to_dec(struct cpu_thread* thread)
{
	struct thread_control_area *tca = get_tca();
//	hprintf("%s: %x %x %x\n", __func__, tca->hdec, tca->vregs->v_dec, tca->active_thread->vdec);
	if (thread->vdec >= 0 && thread->vdec < tca->hdec) {
		tca->vstate |= VSTATE_ACTIVE_DEC;
		mtdec(thread->vdec);
	} else {
		tca->vstate &= ~VSTATE_ACTIVE_DEC;
		mtdec(tca->hdec);
	}
}

void
mthdec(uval32 val)
{
	struct thread_control_area *tca = get_tca();
	tca->hdec = val;
}

void
partition_set_dec(struct cpu_thread *thr, uval32 val)
{
	sync_from_dec();
	if (0 > (sval32)val) {
		val = -1;
	} else {
		/* Positive decrementer, clear dec exception */
		thr->vstate.thread_mode &= ~VSTATE_PENDING_DEC;
		get_tca()->vstate &= ~VSTATE_PENDING_DEC;
	}

	thr->vdec = val;
	thr->vregs->v_dec = val;
	sync_to_dec(thr);
}


sval
xh_dec(struct cpu_thread *thread)
{
	struct thread_control_area *tca = get_tca();


	partition_set_dec(thread, -1);

	if ( (tca->vregs->v_msr & MSR_EE) == 0) {
		thread->vstate.thread_mode |= VSTATE_PENDING_DEC;
		return -1; /* asm code knows to restore silently */
	}

	thread->vstate.thread_mode &= ~VSTATE_PENDING_DEC;
	tca->vstate &= ~VSTATE_PENDING_DEC;
	return insert_exception(thread, EXC_V_DEC);

}

