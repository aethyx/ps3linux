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
#include <lib.h>
#include <hcall.h>
#include <pgalloc.h>
#include <os.h>
#include <idt.h>
#include <pmm.h>
#include <vm.h>
#include <hype.h>
#include <timer.h>
#include <debug.h>
#include <mailbox.h>

#define RPL(a)		(a & 0x3)

static inline sval
raise_exception(struct cpu_thread *thread, int trapno)
{
	struct idt_descriptor *idt_descp;

	idt_descp = get_idt_descriptor(thread, trapno);
	if (idt_descp == NULL)
		return 1;
	if (force_sync_exception(thread, idt_descp, 0) != 0)
		return 1;
	return 0;
}

static inline sval
raise_fault(struct cpu_thread *thread, int trapno, uval32 error_code)
{
	struct idt_descriptor *idt_descp;

	idt_descp = get_idt_descriptor(thread, trapno);
	if (idt_descp == NULL)
		return 1;
	if (force_sync_exception(thread, idt_descp, 1) != 0)
		return 1;
	push(thread, error_code);
	return 0;
}

static inline sval
gen_prot_fault(struct cpu_thread *thread, int trapno, uval32 error_code)
{
	if (handle_partial_seg_fault(thread)) {
		return 0;
	}
#ifdef DEBUG
	hprintf("Exception: trapno 0x%x\n", trapno);
	dump_cpu_state(thread);
#endif
	return raise_fault(thread, trapno, error_code);
}

static inline uval
pop(struct cpu_thread *thread)
{
	uval32 val;
	void *rc;

	rc = get_lpar(thread, &val, thread->tss.gprs.regs.esp, sizeof (val));
	assert(rc != NULL, "invalid stack pointer");	/* "cannot happen" */

	thread->tss.gprs.regs.esp += sizeof (val);
	return val;
}

static void
iret(struct cpu_thread *thread)
{
	uval mask;
	uval error;
	uval fault;
	uval new_cs;
	uval eflags;
	int room;

	/* handle any callbacks before returning */
	if (thread->cb)
		thread->cb(thread);

	/*
	 * Ensure the stack is there and readable.
	 * If there is no stack and we are transitioning to the
	 * guest kernel then we have a problem ...
	 */
	room = 3 + 2;		/* over estimating (for now) */
	mask = ((thread->tss.srs.regs.ss & 0x3) == 0x3) ? PTE_US : 0;
	if (unlikely(!isstackpresent(thread, room, mask, &error, &fault))) {
#ifdef INTR_DEBUG
		hprintf("iret: esp: 0x%lx not mapped (eip 0x%x:0x%lx)\n",
			fault, thread->tss.srs.regs.cs, thread->tss.eip);
#endif
		if (page_fault(thread, error, fault)) {
			assert((mask & PTE_US) == 0, "no guest kernel stack");
			return;
		}
	}

	thread->tss.eip = pop(thread);
	new_cs = pop(thread);
	eflags = pop(thread) & ~EFLAGS_IOPL_MASK;
	/* FIXME! what eflags are controlled by the partition? */

	if (RPL(new_cs) != RPL(thread->tss.srs.regs.cs)) {
		/* if iret to a diff pl, then pop esp/ss */
		uval tmp_esp = pop(thread);

		thread->tss.srs.regs.ss = pop(thread);
		thread->tss.gprs.regs.esp = tmp_esp;
	}

	thread->tss.srs.regs.cs = new_cs;
	thread->tss.eflags = eflags | thread->cpu->os->iopl | EFLAGS_IF;

	if (eflags & EFLAGS_IF)
		enable_intr(thread);
}

/*
 * Handle processor traps/faults. Most of these are reflected
 * to the current partition except for page fault events, these
 * we handle ourselves.
 */
void
exception(struct cpu_thread *thread, uval32 trapno, uval32 error_code)
{
	switch (trapno) {
	case PF_VECTOR:
		page_fault(thread, error_code, get_cr2());
		break;
	case GP_VECTOR:
		gen_prot_fault(thread, trapno, error_code);
		break;
	case DF_VECTOR:
	case TS_VECTOR:
	case NP_VECTOR:
	case SS_VECTOR:
	case AC_VECTOR:
#ifdef DEBUG
		hprintf("Exception: trapno 0x%x\n", trapno);
		dump_cpu_state(thread);
#endif
		raise_fault(thread, trapno, error_code);
		break;
	default:
#ifdef DEBUG
		hprintf("Exception: trapno 0x%x\n", trapno);
		dump_cpu_state(thread);
#endif
		raise_exception(thread, trapno);
		break;
	}
}

/*
 * Handle external (asynchronous) interrupts.
 * For now, only the timer interrupt is virtualized, all other
 * interrupts are forwarded to the I/O (controller) partition.
 */
void
interrupt(struct cpu_thread *thread, uval32 vector)
{
	uval irq = vector - BASE_IRQ_VECTOR;

	pic_mask_and_ack(irq);

	if (likely(irq == TIMER_IRQ)) {	/* timer */
		/* update clock ticks */
		ticks++;

		/* keep track of decrementer and reschedule if necessary */
		if (unlikely(decrementer-- == 0)) {
			decrementer = DECREMENTER;
			thread->preempt = CT_PREEMPT_RESCHEDULE;
		} else {
			thread->preempt = CT_PREEMPT_CEDE;
		}
		pic_enable(irq);
	} else {
		struct cpu_thread *ct = &ctrl_os->cpu[0]->thread[0];
		struct mailbox *mbox = ct->mbox;

		if (mbox) {
			mbox->irq_pending |= 1 << irq;
			if (ct->preempt == 0)	/* schedule interrupt */
				ct->preempt = CT_PREEMPT_CEDE;
		}
	}
}

/*
 * Handle (synchronous) user traps. These are all reflected
 * to the current partition, except for the hypervisor reserved
 * interrupts when they are issued by the partition OS. When
 * a hypervisor interrupt is generate by a partition application
 * (ring 3) it is always reflected to the guest OS.
 */
void
trap(struct cpu_thread *thread, uval32 trapno)
{
	uval cpl = RPL(thread->tss.srs.regs.cs);

	if (likely(trapno >= BASE_HCALL_VECTOR && cpl < 3)) {
		switch (trapno) {
		case HCALL_VECTOR:	/* hcall */
			hcall(thread);
			break;
		case HCALL_VECTOR_IRET:	/* iret */
			iret(thread);
			break;
		default:
			hprintf("%s: int 0x%ulx in reserved range - ignored\n",
				__func__, trapno);
			break;
		}
		return;
	}
	raise_exception(thread, trapno);
}
