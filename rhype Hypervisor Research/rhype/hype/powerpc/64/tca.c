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
#include <thread_control_area.h>
#include <debug.h>
#include <slb.h>
#include <float.h>
#include <hype.h>
#include <mmu.h>
#include <atomic.h>
#include <rcu.h>
#include <objalloc.h>

static void
save_sprs(struct cpu_thread *thread)
{
	thread->reg_tb = mftb();
	thread->reg_dec = mfdec();

	thread->reg_sprg[0] = mfsprg0();
	thread->reg_sprg[1] = mfsprg1();
	thread->reg_sprg[2] = mfsprg2();
	thread->reg_sprg[3] = mfsprg3();

	thread->reg_dar = mfdar();
	thread->reg_dsisr = mfdsisr();
}

static void
restore_sprs(struct cpu_thread *thread)
{
	uval64 tb_delta;

	mtsprg0(thread->reg_sprg[0]);
	mtsprg1(thread->reg_sprg[1]);
	mtsprg2(thread->reg_sprg[2]);
	mtsprg3(thread->reg_sprg[3]);

	mtdar(thread->reg_dar);
	mtdsisr(thread->reg_dsisr);

	/* adjust the dec value to account for cycles while not
	 * running this OS */
	tb_delta = mftb() - thread->reg_tb;
	thread->reg_dec -= tb_delta;
	mtdec(thread->reg_dec);
}

struct cpu_thread *
preempt_thread(struct cpu_thread *thread, uval timer)
{
	struct thread_control_area *tca = get_tca();
	int preempt_cpu = 0;

	/* if this was a yield, release the CPU */
	if (thread->preempt == CT_PREEMPT_YIELD_SELF ||
	    thread->preempt == CT_PREEMPT_YIELD_DIRECTED || timer) {
		preempt_cpu = 1;
	}

	if (preempt_cpu) {
		/* We are going to preempt this cpu. Preempt this
		 * thread and go into the cpu idle state */

		save_sprs(thread);
		save_slb(thread);
		save_float(thread);

		tca->active_thread = NULL;

		cpu_idle();
		assert(0, "point of no return - should on get here");
	}

	if (thread->preempt == CT_PREEMPT_CEDE) {
		/* todo - check for pending interrupts and resume
		 * thread if there are any */
		lock_acquire(&thread->lock);
		assert(thread->state == CT_STATE_EXECUTING,
		       "should be executing at this point");
		thread->state = CT_STATE_WAITING;
		DEBUG_MT(DBG_TCA, "preempt_thread: CT_PREEMPT_CEDE "
			 "forced transition to CT_STATE_WAITING\n");
		lock_release(&thread->lock);
	}
	DEBUG_MT(DBG_TCA, "preempt_thread: making online "
		 "thread dormant dec=%x, hdec=%x\n",
		 mfdec(), mfhdec());

	/* we are ceding this threads cycles - go dormant */
	save_sprs(thread);
	save_slb(thread);
	save_float(thread);
	enter_dormant_state();

	assert(0, "point of no return - should not get here\n");
	return NULL;
}

static int
check_and_deliver_externals(struct cpu_thread *thread)
{
	int external_present = !xirqs_empty(thread);
	const uval srr_mask = ~(MSR_IR | MSR_DR | MSR_FE0 | MSR_PR |
				MSR_FE1 | MSR_EE | MSR_RI);

	if (external_present) {
		/* can we deliver the external now? */
		if (thread->reg_hsrr1 & MSR_EE) {
			DEBUG_MT(DBG_EE, "%s: LPAR[0x%x] direct\n",
				 __func__, thread->cpu->os->po_lpid);
			uval vector = 0x500;

			thread->reg_srr0 = thread->reg_hsrr0;
			/* SRR1[33:36] & SRR1[42:47] set to 0 */
			thread->reg_srr1 = thread->reg_hsrr1 &
				~0x00000000783f0000;

			thread->reg_hsrr0 = vector;
			thread->reg_hsrr1 = thread->reg_hsrr1 & srr_mask;

			thread->reg_hsrr1 = srr1_set_sf(thread->reg_hsrr1);

			/* TODO - this needs to be moved to the proper
			 * place - we should only turn off LPCR[MER] if
			 * there are no more externals that have not been
			 * presented to the OS */

			/* turn off MER if we can now deliver
			 * the interrupt and it is currently
			 * on */
			thread_set_MER(thread, 0);
		} else {
			DEBUG_MT(DBG_EE, "%s: LPAR[0x%x] MER\n", __func__,
				 thread->cpu->os->po_lpid);
			/* defer the delivery using MER */
			thread_set_MER(thread, 1);
		}
	} else {
		/*
		 * So I was very confused as to why this could ever
		 * happen.  Well it could happen it while the MSR[EE]
		 * = 0 the external event queue is emptied by the
		 * LPAR.
		 *
		 * Q: Should we reset here or when the last item is queued?
		 */

		thread_set_MER(thread, 0);
	}
	return external_present;
}

extern void sys_reset(void);

void
enter_dormant_state(void)
{
	struct thread_control_area *tca = get_tca();
	lock_t *dormant_lock = &tca->dormant_lock;
	uval32 ctrl;

	DEBUG_MT(DBG_TCA, "enter_dormant_state\n");

	lock_acquire(dormant_lock);

	if (tca->state == TCA_STATE_ACTIVE) {
		tca->state = TCA_STATE_DORMANT;

		lock_release(dormant_lock);

		ctrl = 0x00C00000;
		ctrl &= ~(0x00800000 >> tca->thread_index);

		mtctrl(ctrl);
	} else if (tca->state == TCA_STATE_BLOCK_DORMANT) {
		DEBUG_MT(DBG_TCA, "taking the blocked dormant path\n");
		tca->state = TCA_STATE_ACTIVE;
		/* pretend a mtcrl woke us up */
		mtsrr1(0x0000000000280000);
		sys_reset();
	} else {
		assert(0, "how did this happen?\n");
	}
}

void
exit_dormant_state(void)
{
	uval8 wakeup_reason = (mfsrr1() >> 19) & 0x7;
	struct thread_control_area *tca = get_tca();
	struct cpu_thread *thread = tca->active_thread;

	DEBUG_MT(DBG_TCA, "%s: wakeup reason %d, dec=%x, hdec=%x "
		 "thread_state=%x\n",
		 __func__, wakeup_reason, mfdec(), mfhdec(),
		 thread->state);

	lock_acquire(&thread->lock);

	assert(tca->state == TCA_STATE_DORMANT, "how did this happen?\n");
	tca->state = TCA_STATE_ACTIVE;

	/* an external int woke us up - go process it */
	if (wakeup_reason == 0x4) {
		if (check_and_deliver_externals(thread)) {
			/* the external was for us - resume the thread */
			DEBUG_MT(DBG_TCA, "%s: resuming thread due to EXT\n",
				 __func__);
			assert(thread->state == CT_STATE_WAITING,
			       "how did this happen?\n");
			thread->state = CT_STATE_EXECUTING;
			lock_release(&thread->lock);
			resume_thread_asm(tca->active_thread);
		} else {
			/*
			 * the external was for someone else - go idle
			 * so they can get scheduled
			 */
			DEBUG_MT(DBG_TCA, "%s: preempting due to an "
				 "EXT for someone else\n", __func__);
			lock_release(&thread->lock);
			cpu_idle();
		}
	}

	/* this cpu is preempting - enter the idle loop */
	if (thread->cpu->state == C_STATE_PREEMPTING) {
		lock_release(&thread->lock);
		cpu_idle();
	}

	/* this thread is offline - enter the idle loop */
	if (thread->state == CT_STATE_OFFLINE) {
		lock_release(&thread->lock);
		cpu_idle();
	}

	/* a DEC woke us up - simply resume the thread so it can handle it */
	if (wakeup_reason == 0x3) {
		DEBUG_MT(DBG_TCA, "%s: resuming thread due to DEC\n", __func__);
		assert(thread->state == CT_STATE_WAITING,
		       "how did this happen?");
		thread->state = CT_STATE_EXECUTING;
		lock_release(&thread->lock);
		resume_thread_asm(tca->active_thread);
	}

	/* an hdec woke us up - go into cpu_idle to get the next os
	 * dispatched */
	if (wakeup_reason == 0x5) {
		assert(thread->state == CT_STATE_EXECUTING,
		       "how did this happen?");
		lock_release(&thread->lock);
		cpu_idle();
	}

	assert(0, "need to write code for this case");
}

void
wakeup_partner_thread(void)
{
#if THREADS_PER_CPU > 1
	thread_control_area *partner_tca = get_tca()->partner_tca;
	lock_t *partner_dormant_lock = &partner_tca->dormant_lock;

	lock_acquire(partner_dormant_lock);
	uval8 partner_state = partner_tca->state;

	if (partner_state == TCA_STATE_ACTIVE) {
		partner_state = TCA_STATE_BLOCK_DORMANT;
	} else if (partner_state == TCA_STATE_DORMANT) {
		uval32 ctrl = 0;
		uval32 active_mask = (0x80000000 >> partner_tca->thread_index);

		for (;;) {
			/* loop until the thread is really dormant */
			ctrl = mfctrl();
			if (!(ctrl & active_mask)) {
				/* it is really dormant, so wake it up */
				mtctrl(0x00C00000);
				break;
			}
		}
	}

	lock_release(partner_dormant_lock);
#endif /* THREADS_PER_CPU > 1 */
}

void __attribute__ ((noreturn))
resume_thread(void)
{
	struct thread_control_area *tca = get_tca();
	struct cpu_thread *thread = tca->active_thread;
	int primary_thread = (tca->thread_index == 0);

#ifdef TRACK_LPAR_ENTRY
	hprintf("%s[%u]: Enter LPAR:[0x%x] @ 0x%016lx\n",
		__func__, !primary_thread, thread->cpu->os->po_lpid,
		thread->reg_hsrr0);
#endif
	if (primary_thread) {
		/* primary thread work goes here */

#ifdef HAS_TAGGED_TLB
		if (thread->cpu->tlb_flush_pending) {
			hprintf("%s: flushing TLB before resume\n", __func__);
			tlbia();
			thread->cpu->tlb_flush_pending = 0;
		}
#else
		tlbia();
#endif

		ptesync();
		mtsdr1(thread->cpu->reg_sdr1);
		mtlpidr(0x1f);	/* reserved LPID = 0x1f */
		slbia();
		sync_with_partner_thread();
		sync_with_partner_thread();
		mtlpidr(thread->cpu->os->po_lpid_tag);
		restore_large_page_selection(thread);
		sync_with_partner_thread();
	} else {
		/* secondary thread work goes here */
		sync_with_partner_thread();
		slbia();
		sync_with_partner_thread();
		sync_with_partner_thread();
	}

	/* Set RMOR. LPCR, etc... */
	imp_switch_to_thread(thread);

	restore_sprs(thread);
	restore_float(thread);
	restore_slb(thread);

	lock_acquire(&thread->lock);

	if (thread->state == CT_STATE_OFFLINE) {
		assert(0, "this should never happen\n");
		/* change the dec so it won't wakeup an offline thread */
		mtdec(0x7fffffff);
		lock_release(&thread->lock);
		enter_dormant_state();
	}

	if (thread->state == CT_STATE_WAITING) {
		/* the thread is waiting - see if a decr wake it up */
		/* TODO - check if any external interrupts are present */
		if (mfdec() & 0x80000000) {
			thread->state = CT_STATE_EXECUTING;
			DEBUG_MT(DBG_TCA, "%s: dec forced transition from WAITING "
				 "to EXECUTING " "dec = %x\n",
				 __func__, mfdec());
		} else {
			lock_release(&thread->lock);
			enter_dormant_state();
		}
	}

	check_and_deliver_externals(thread);

	thread->preempt = 0;

	lock_release(&thread->lock);
	DEBUG_MT(DBG_TCA, "resume_thread_asm(thread %p)\n", thread);

	resume_thread_asm(thread);
}

uval
tca_init(struct thread_control_area *tca,
	 struct hype_control_area *hca,
	 uval stack, uval idx)
{
#if THREADS_PER_CPU > 1
#error "this does not work yet"
#endif
	struct cpu_control_area *cca;

	assert(idx == 0, "single threaded only\n");

	memset(tca, 0, sizeof (*tca));

	cca = (struct cpu_control_area *)(uval)(tca - sizeof (*cca));
	cca_init(cca);

	tca->eye_catcher = 0x54434120;
	tca->thread_index = idx;
	tca->state = TCA_STATE_INIT;
	tca->cca = cca;
	tca->primary_tca = tca;
	tca->hypeStack = stack;
	tca->cached_hca = hca;
	tca->hypeToc = get_toc();

	lock_init(&tca->dormant_lock);

	mthsprg0((uval)tca);
	mtr13((uval)tca);

	return 1;
}

/*
 * This table will allow us to go from the PIR register to a TCA, This
 * is required for processors that do not save any state on CPU/Thread
 * suspend, we don't want any fancy data structs here because we need
 * to get here from assembler with 0 processor state.
 */

struct thread_control_area **tca_table;
static uval tca_table_size;
uval
tca_table_init(uval n)
{
	tca_table = halloc(n * sizeof (tca_table));
	assert(tca_table != NULL, "failed to alloc tca_table\n");

	tca_table_size = n;
	return 1;
}

uval
tca_table_enter(uval e, struct thread_control_area *tca)
{
	assert(e < tca_table_size, "bad index\n");
	if (e > tca_table_size) {
		return 0;
	}
	tca_table[e] = tca;

	return 1;
}
