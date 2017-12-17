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
 *
 */
#include <config.h>
#include <lib.h>
#include <cpu_thread.h>
#include <mailbox.h>
#include <pgalloc.h>
#include <idt.h>
#include <pmm.h>
#include <mmu.h>
#include <vm.h>
#include <hype.h>
#include <debug.h>
#include <h_proto.h>

/*
 * 'IF' field in a thread's mailbox, controls represents
 * the IF eflags bit that the thread wants set. Since
 * interrupts always go through the hv, we just look at this
 * field before delivering interrupts to the thread
 */
static int is_intr_enabled(struct cpu_thread *thread) __attribute__ ((unused));
static int
is_intr_enabled(struct cpu_thread *thread)
{
	struct mailbox *mbox = thread->mbox;

	if (mbox == NULL)
		return 0;
	return mbox->IF;
}

void
enable_intr(struct cpu_thread *thread)
{
	struct mailbox *mbox = thread->mbox;

	if (mbox != NULL)
		mbox->IF = 1;
}

void
disable_intr(struct cpu_thread *thread)
{
	struct mailbox *mbox = thread->mbox;

	if (mbox != NULL)
		mbox->IF = 0;
}

/*
 * Push value onto the stack pointed to by thread->tss.gprs.regs.esp
 */
void
push(struct cpu_thread *thread, uval32 val)
{
	void *rc;

	thread->tss.gprs.regs.esp -= sizeof (val);

	rc = put_lpar(thread, &val, thread->tss.gprs.regs.esp, sizeof (val));
	assert(rc != NULL, "invalid stack pointer");	/* "cannot happen" */
}

/*
 * Push the current m/c state as expected by
 * an exception handler onto the stack
 */
static uval
push_exception_state(struct cpu_thread *thread, int pl_change, int except)
{
	int room = 3 + 2 * pl_change + except;
	uval old_ss = 0;
	uval old_esp = 0;
	uval16 new_ss;
	uval32 new_esp;
	uval mask;
	uval error;
	uval fault;

	if (pl_change) {
		/* on a pl change, load ss/esp from the partition sys_stack */
		new_ss = thread->tss.ss2;
		new_esp = thread->tss.esp2;

		old_ss = thread->tss.srs.regs.ss;
		old_esp = thread->tss.gprs.regs.esp;

		thread->tss.srs.regs.ss = new_ss;
		thread->tss.gprs.regs.esp = new_esp;
	}

	/*
	 * Ensure the stack is there and writeable.
	 * If there is no stack and we are transitioning to the
	 * guest kernel then we have a problem ...
	 */
	if ((thread->tss.srs.regs.ss & 0x3) == 0x3) {
		mask = PTE_US | PTE_RW;
	} else {
		mask = PTE_RW;
	}
	if (unlikely(!isstackpresent(thread, room, mask, &error, &fault))) {
#ifdef INTR_DEBUG
		hprintf("esp: 0x%lx not mapped (eip 0x%x:0x%lx)\n",
			fault, thread->tss.srs.regs.cs, thread->tss.eip);
#endif
		if (page_fault(thread, error, fault)) {
			assert((mask & PTE_US) == 0, "no guest kernel stack");
			return 1;
		}
	}

	/* simulate an exception */
	if (pl_change) {
		push(thread, old_ss);
		push(thread, old_esp);
	}

	assert((thread->tss.eflags & EFLAGS_IOPL_MASK) ==
	       thread->cpu->os->iopl, "thread not at partition IOPL");

	/*
	 * Ensure that the interrupts are always enabled.
	 * Interrupts to the partition are controlled through the mailbox.
	 */
	push(thread, thread->tss.eflags | EFLAGS_IF);
	push(thread, (uval)thread->tss.srs.regs.cs);
	push(thread, thread->tss.eip);

	return 0;
}

/*
 * Force a synchronous exception
 */
uval
force_sync_exception(struct cpu_thread *thread,
		     struct idt_descriptor *idt_descp, int except)
{
	uval pl_change;

	/* push current m/c state to stack */
	pl_change = pl_change_required(thread, idt_descp);
	if (push_exception_state(thread, pl_change, except))
		return 1;

	/* set thread state to run the exception handler */
	thread->tss.srs.regs.cs = get_intr_handler_segment(idt_descp);
	thread->tss.eip = get_intr_handler_offset(idt_descp);
	thread->tss.eflags &= ~EFLAGS_TF;

	/* enable/disable intr's for this thread based on the gate type */
	if (get_idt_gate_type(idt_descp) == INTR_GATE)
		disable_intr(thread);

	return 0;
}

/*
 * A page fault is like a normal sync exception with
 * an additional error code on the stack.
 */
uval
force_page_fault(struct cpu_thread *thread, uval error_code, uval cr2)
{
	struct idt_descriptor *idt_descp;

	idt_descp = get_idt_descriptor(thread, PF_VECTOR);
	if (idt_descp == NULL)
		return 1;

	if (force_sync_exception(thread, idt_descp, 1) == 0) {
		push(thread, error_code);	/* error code */
		thread->reg_cr2 = cr2;	/* fault address */
		return 0;
	}
	return 1;
}

/*
 * Determine which interrupt to deliver to the partition.
 * The current implementation rotates over the pending interrupts
 * to ensure fairness and prevent the clock interrupt from locking
 * out all other interrupts on the simulator.
 */
static inline sval
external_interrupt(struct cpu_thread *thread)
{
	struct mailbox *mbox = thread->mbox;
	uval mask;
	sval i;

	if (mbox == NULL)
		return -1;
	if (mbox->IF == 0)
		return -1;

	if ((mask = mbox->irq_pending & ~mbox->irq_masked) == 0)
		return -1;

	for (i = 0; i < NR_INTERRUPT_HANDLERS; i++) {
		int irq = (i + thread->lastirq + 1) % NR_INTERRUPT_HANDLERS;

		if (mask & (1 << irq))
			return thread->lastirq = irq;
	}
	return -1;
}

/*
 * Deliver interrupt to the partition
 */
void
deliver_intr_exception(struct cpu_thread *thread)
{
	struct idt_descriptor *idt_descp;
	struct mailbox *mbox = thread->mbox;
	uval vector;
	sval irq;

	if (mbox == NULL)
		return;

	if ((irq = external_interrupt(thread)) == -1)
		return;

	vector = irq + BASE_IRQ_VECTOR;
	idt_descp = get_idt_descriptor(thread, vector);
	if (idt_descp == NULL)
		return;

	if (force_sync_exception(thread, idt_descp, 0) != 0)
		return;

	mbox->irq_masked |= 1 << irq;
	mbox->irq_pending &= ~(1 << irq);
}

uval
clear_exception_state(struct cpu_thread *thread __attribute__ ((unused)))
{
	return 0;
}
