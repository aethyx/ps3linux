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
#include <hypervisor.h>
#include <cpu_control_area.h>
#include <thread_control_area.h>
#include <hype.h>
#include <debug.h>
#include <sim.h>
#include <objalloc.h>

static uval hdec_slot_time;

void __attribute__ ((noreturn))
cpu_idle(void)
{
	struct thread_control_area *tca = get_tca();
	struct cpu_control_area *cca = get_cca();
	struct cpu *old_cpu;
	struct cpu *new_cpu;
	int primary_thread = (tca->thread_index == 0);
	uval32 hdec_count = hdec_slot_time;

	DEBUG_MT(DBG_TCA, "%s: enter: threadId %d  cpuId %d activeCpu %p\n",
		 __func__, tca->thread_index,
		 cca->thread_count, cca->active_cpu);

	/* mark our TCA as active to prevent suprious blocked dormancies */
	tca->state = TCA_STATE_ACTIVE;

	assert(cca->active_cpu != NULL, "no active CPU\n");
	cca->active_cpu->state = C_STATE_PREEMPTING;

	wakeup_partner_thread();

	/* clear the old thread binding */
	tca->active_thread = NULL;

	/* TODO - move this syncronization back to the thread level???? */
	if (primary_thread) {
		/* primary thread path goes here */
		sync_with_partner_thread();
	} else {
		/* secondary thread path goes here */
		sync_with_partner_thread();
	}

	if (primary_thread) {
		/* no need for lock here - only the primary thread is
		 * touching it */
		cca->active_cpu->state = C_STATE_PREEMPTED;

		/* both threads are together now, clear the old cpu
		 * binding */
		old_cpu = cca->active_cpu;
		cca->active_cpu = NULL;

		/* TODO - change this to sched_next_cpu */
		new_cpu = sched_next_os(old_cpu);

		if (new_cpu) {
			cca->active_cpu = new_cpu;
		} else {
			/* TODO - cpu idling should go somewhere around here */
			cca->active_cpu = old_cpu;
		}

		cca->active_cpu->state = C_STATE_EXECUTING;

		mthdec(hdec_count);

		DEBUG_MT(DBG_TCA, "%s: dispatching LPAR[0x%x] cpu %p\n",
			 __func__, cca->active_cpu->os->po_lpid,
			 cca->active_cpu);

#if THREADS_PER_CPU > 1
		sync_with_partner_thread();
#endif
	} else {
#if THREADS_PER_CPU > 1
		sync_with_partner_thread();
#else
		assert(0, "never get here\n");
#endif /* THREADS_PER_CPU > 1 */
	}

	/* update the struct cpu_thread binding */
	tca->active_thread = &cca->active_cpu->thread[tca->thread_index];

	resume_thread();
}

uval
cca_init(struct cpu_control_area *cca)
{
	cca->eye_catcher = 0x43434120;
	cca->thread_count = 1;
	cca->active_cpu = NULL;
	cca->tlb_index_randomizer = 0;
	lock_init(&cca->tlb_lock);

	if (hdec_slot_time == 0) {
		hdec_slot_time = 0x00008000;
		if (onsim()) {
			hdec_slot_time <<= 4;
		}
	}
	return 1;
}

static struct cpu_control_area **cca_table;
static uval cca_table_size;
uval
cca_table_init(uval n)
{
	cca_table = halloc(n * sizeof (cca_table));
	assert(cca_table != NULL, "failed to alloc tca_table\n");

	cca_table_size = n;
	return 1;
}

uval
cca_table_enter(uval e, struct cpu_control_area *cca)
{
	assert(e < cca_table_size, "bad index\n");
	if (e > cca_table_size) {
		return 0;
	}
	cca_table[e] = cca;
	return 1;
}

uval
cca_hard_start(struct cpu *cpu, uval i, uval pc)
{
	struct cpu_control_area *cca;

	if (i < cca_table_size) {
		cca = (struct cpu_control_area *)cca_table[i];

		if (cca->active_cpu == NULL) {
			cca->active_cpu = &cpu[i];
			cpu->thread->reg_hsrr0 = pc;
			cpu->thread->reg_hsrr1 = (MSR_SF | MSR_ME);
			cpu->thread->reg_dec = 0xff0000;

			return i;
		}
	}
	return 0;
}
		
