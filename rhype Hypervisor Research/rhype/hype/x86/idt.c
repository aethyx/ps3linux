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
/*
 * IDT Setup 
 */

#include <config.h>
#include <lib.h>
#include <pgalloc.h>
#include <asm.h>
#include <lpar.h>
#include <pmm.h>
#include <hcall.h>
#include <idt.h>
#include <vm.h>
#include <hype.h>
#include <h_proto.h>
#include <cpu_thread.h>

extern uval trap_handlers[];
extern uval exception_handlers[];
extern uval interrupt_handlers[];

static void
set_trap_gate(struct descriptor *descr_p,
	      uval16 selector, uval32 offset, uval16 flags)
{
	descr_p->word0 = ((uval32)selector << 16) | (offset & 0xffff);
	descr_p->word1 = (offset & 0xffff0000) | flags;
}

static void
set_intr_gate(struct descriptor *descr_p, uval handler, uval dpl)
{
	uval flags = 0x8e00 | ((dpl & 0x3) << 13);

	set_trap_gate(descr_p, __HV_CS, handler, flags);
}

void
idt_init(struct cpu_thread *thread)
{
	uval i;

	/*
	 * Set processor trap/exception handlers.
	 * The HV always runs interrupts disabled, so we use
	 * intr gate for all entry points.
	 */
	for (i = 0; i < NR_EXCEPTIONS_HANDLERS; i++) {
		/* DB and BP exceptions are needed for client debugging */
		uval pl = ((i == DB_VECTOR) || (i == BP_VECTOR)) ? 3 : 0;

		set_intr_gate(&thread->idt[i], exception_handlers[i], pl);
	}

	/* set interrupt handlers */
	for (i = 0; i < NR_INTERRUPT_HANDLERS; i++) {
		set_intr_gate(&thread->idt[BASE_IRQ_VECTOR + i],
			      interrupt_handlers[i], 0);
	}

	/* unless caught, all traps are forwarded to the partition */
	for (i = 0; i < NR_TRAP_HANDLERS; i++) {
		set_intr_gate(&thread->idt[BASE_TRAP_VECTOR + i],
			      (uval)trap_handlers[i], 3);
	}

	thread->idtr.base = (uval32)thread->idt;
	thread->idtr.size = sizeof (thread->idt) - 1;

	memset(thread->idt_info, 0, sizeof (thread->idt_info));
}

struct idt_descriptor *
get_idt_descriptor(struct cpu_thread *thread, uval vector)
{
	if (vector >= NR_IDT_ENTRIES)
		return NULL;
	return &thread->idt_info[vector];
}

uval
pl_change_required(struct cpu_thread *thread,
		   struct idt_descriptor *descp __attribute__ ((unused)))
{
	uval cpl = thread->tss.srs.regs.cs & 0x3;

	return cpl == 3 ? 1 : 0;
}

sval
h_idt_entry(struct cpu_thread *thread, uval index, uval word0, uval word1)
{
	uval32 offset;
	uval16 segment,
	       flags;
	uval seg_idx;

#ifdef IDT_DEBUG
	hprintf("%s: lpid 0x%x: index %ld, word0 0x%lx, word1 0x%lx\n",
		__func__, thread->cpu->os->po_lpid, index, word0, word1);
#endif
	if (index >= NR_IDT_ENTRIES) {
		return H_Parameter;
	}

	offset = (word1 & 0xffff0000) | (word0 & 0x0000ffff);
	segment = word0 >> 16;
	flags = word1 & 0x0000ffff;

	/*
	 * For now, we only support 32-bit task, interrupt, and trap gates.
	 */
	if (((flags & GATE_MASK) != TASK_GATE) &&
	    ((flags & GATE_MASK) != INTR_GATE) &&
	    ((flags & GATE_MASK) != TRAP_GATE)) {
		return H_Parameter;
	}

	/*
	 * The segment must be in the client-controllable range in the gdt.
	 * Entries in that range are enforced to be in ring 2 or 3.  We don't
	 * check the validity of the segment table entry or the offset.  If
	 * the entry or offset is invalid, we'll take a protection fault which
	 * will be passed on to the client.
	 */
	seg_idx = (segment >> 3);
	if ((seg_idx < GDT_ENTRY_MIN) || (seg_idx >= GDT_ENTRY_COUNT)) {
		return H_Parameter;
	}

	thread->idt_info[index].offset = offset;
	thread->idt_info[index].segment = segment;
	thread->idt_info[index].flags = flags;

	/*
	 * We can install the entry directly in the real idt if it's in the
	 * "trap" range of idt indices and if the entry is a trap gate.  We
	 * can't allow interrupt gates to pass straight through because we
	 * have to enter the client with interrupts disabled.
	 */
	if ((index >= BASE_TRAP_VECTOR) && (index < BASE_HCALL_VECTOR)) {
		if ((flags & GATE_MASK) == TRAP_GATE) {
			thread->idt[index].word0 = word0;
			thread->idt[index].word1 = word1;
		} else {
			uval ti = index - BASE_TRAP_VECTOR;

			/* restore default handler */
			set_intr_gate(&thread->idt[index],
				      (uval)trap_handlers[ti], 3);
		}
	}

	return H_Success;
}

sval
h_dt_entry(struct cpu_thread *thread, uval type,
	   uval index, uval word0, uval word1)
{
	switch (type) {
	case H_DT_GDT:
		return h_gdt_entry(thread, index, word0, word1);
	case H_DT_IDT:
		return h_idt_entry(thread, index, word0, word1);
	default:
		return H_Parameter;
	}
}
