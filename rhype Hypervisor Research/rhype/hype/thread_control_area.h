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

#ifndef _THREADCONTROLAREA_H
#define _THREADCONTROLAREA_H

#include <config.h>
#include <types.h>
#include <hype.h>
#include <cpu.h>
#include <cpu_thread.h>
#include <hype_control_area.h>
#include <cpu_control_area.h>
#include <os.h>
#include <hv_regs.h>
#include <stack.h>

#define TCA_STATE_DORMANT       0x00
#define TCA_STATE_BLOCK_DORMANT 0x01
#define TCA_STATE_ACTIVE        0x02
#define TCA_STATE_INIT          0xFF

struct thread_control_area {
	uval32 eye_catcher;
	uval32 thread_index;
	lock_t dormant_lock;
	volatile uval32 state;

	/* 64 bit aligned here */
	/* uval members here */
	struct cpu_control_area *cca;
	struct thread_control_area *primary_tca;
	struct thread_control_area *partner_tca;
	struct cpu_thread *active_thread;
	struct hype_control_area *cached_hca;
	volatile uval partner_sync_count;
	uval hypeStack;
	uval hypeToc;
	uval cachedR13;
	uval cachedR14;
	uval lasthcall;
	uval lastfew[32];	/* does this position mean anything */
};

extern struct cpu_thread *preempt_thread(struct cpu_thread *, uval);
extern void resume_thread(void) __attribute__ ((noreturn));
extern void resume_thread_asm(struct cpu_thread *) __attribute__ ((noreturn));
extern void enter_dormant_state(void);
extern void exit_dormant_state(void);
extern void wakeup_partner_thread(void);
extern void thread_init(struct thread_control_area *tca);
extern uval tca_init(struct thread_control_area *tca,
		     struct hype_control_area *hca,
		     uval stack, uval idx);
extern uval tca_table_init(uval n);
extern uval tca_table_enter(uval e, struct thread_control_area *loc);

static __inline__ struct cpu_control_area *
get_cca(void)
{
	return ((struct thread_control_area *)get_tca())->cca;
}

static __inline__ struct thread_control_area *
tca_from_stack(uval stack)
{
	uval bottom = stack_bottom(stack);
	struct thread_control_area *tca;

	tca = (struct thread_control_area *)(bottom - sizeof (*tca));

	return tca;
}

#if THREADS_PER_CPU > 1
static __inline__ void
sync_with_partner_thread(void)
{
	struct thread_control_area *this_tca = get_tca();
	struct thread_control_area *partner_tca = this_tca->partner_tca;
	uval my_sync_count = 0;

	/* FIXME - remove when we turn on the second thread */
	if (this_tca->partner_tca == NULL) {
		DEBUG_MT(DBG_TCA, "TCA = %p", this_tca);
		assert(0, "single thread more no longer supported");
		return;
	}
	DEBUG_MT(DBG_TCA, "%d sync start\n", this_tca->thread_index);

	/* increment my count */
	my_sync_count = ++this_tca->partner_sync_count;

	/* wake for my partner to reach the same count */
	while (partner_tca->partner_sync_count < my_sync_count) ;

	DEBUG_MT(DBG_TCA, "%d sync complete\n", this_tca->thread_index);
}

/*
 * This can blindly just start both threads, since the current thread
 * is obviously running, and the other one needs to be woken up.
 */
static __inline__ void
start_partner_thread(void)
{
	mtctrl(0x00C00000);
}
#else
static __inline__ void
sync_with_partner_thread(void)
{
	return;
}
static __inline__ void
start_partner_thread(void)
{
	return;
}

#endif /* THREADS_PER_CPU > 1 */

#endif /* _THREADCONTROLAREA_H */
