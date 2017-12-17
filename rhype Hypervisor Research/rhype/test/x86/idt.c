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

/* idt.c
 * setup idt for test OSs
 *
 */

/* Most of the functionality is present in hype/x86/idt.c too, but
 * these do not really belong in lib/, so they don't share code.
 */

#include <config.h>
#include <lib.h>
#include <asm.h>
#include <test.h>
#include <hcall.h>
#include <mailbox.h>
#include <mmu.h>
#include <regs.h>
#include <idt.h>

/* *INDENT-OFF* */
struct mailbox mbox __attribute__ ((weak)) = {
	.IF = 0,		/* IF - cli */
	.irq_pending = 0,	/* no interrupts pending */
	.irq_masked = ~0,	/* mask all interrupts */
	.ticks = 0		/* global tick count */
};
/* *INDENT-ON* */

/* *INDENT-OFF* */
struct descriptor idt[NR_IDT_ENTRIES] __attribute__ ((aligned(32))) = {{0,0},};
/* *INDENT-ON* */

extern void default_handler(void);
extern void async_handler(void);
extern void pfault_handler(void);
extern void gdb_handler(void);

void
set_trap_gate(uval entry, uval16 selector, uval32 offset, uval16 flags)
{
	sval rc;

	idt[entry].word0 = (offset & 0x0000ffff) | ((uval32)selector << 16);
	idt[entry].word1 = (offset & 0xffff0000) | flags;
	rc = hcall_dt_entry(NULL, H_DT_IDT,
			    entry, idt[entry].word0, idt[entry].word1);
	assert(rc == H_Success, "Failed to set IDT entry");
}

void
set_intr_gate(uval entry, uval handler, uval dpl)
{
	uval flags = 0x8e00 | ((dpl & 0x3) << 13);

	set_trap_gate(entry, __GUEST_CS, handler, flags);
}

void
setup_idt(void)
{
	int i;
	sval rc;

	rc = hcall_set_mbox(NULL, (uval)&mbox);
	assert(rc == H_Success, "Failed to set mailbox");

	for (i = 0; i < NR_IDT_ENTRIES; i++) {
		set_intr_gate(i, (uval)default_handler, 2);
	}

	set_intr_gate(PF_VECTOR, (uval)pfault_handler, 2);

	set_intr_gate(XIR_VECTOR, (uval)async_handler, 2);
	mbox.irq_masked &= ~(1 << XIR_IRQ);	/* enable XIR interrupts */

#ifdef USE_GDB_STUB
	set_intr_gate(DB_VECTOR, (uval)gdb_handler, 2);
	set_intr_gate(BP_VECTOR, (uval)gdb_handler, 2);
#endif
}

void
do_pfault(uval ax, uval cx, uval dx, uval bx, uval ebp, uval esi, uval edi,
	  uval ret, uval cr2, uval err, uval eip, uval cs, uval eflags)
{
	/* these are currently unused.. but could be */
	ax = ax;
	cx = cx;
	dx = dx;
	bx = bx;
	ebp = ebp;
	esi = esi;
	edi = edi;
	ret = ret;

	hprintf("pagefault:\n");
	hprintf("\tcr2: 0x%lx\n", cr2);
	hprintf("\terr: 0x%lx\n", err);
	hprintf("\teflags: 0x%lx\n", eflags);
	hprintf("\tcs : eip:  0x%lx : 0x%lx\n", cs, eip);
	halt();
}
