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
 *
 * Thread specific defines.
 *
 */
#ifndef __HYPE_X86_CPU_THREAD_H__
#define __HYPE_X86_CPU_THREAD_H__

#include <config.h>
#include <types.h>
#include <ipc.h>
#include <mailbox.h>
#include <atomic.h>
#include <pgalloc.h>
#include <gdt.h>
#include <idt.h>
#include <regs.h>
#include <xir.h>

/* thread state constants */
#define CT_STATE_OFFLINE     0x00
#define CT_STATE_WAITING     0x01
#define CT_STATE_EXECUTING   0x02

/* preempt reason constants */
#define CT_PREEMPT_RESCHEDULE		0x01	/* go through schedulers */
#define CT_PREEMPT_YIELD_SELF		0x02	/* yield - not-directed */
#define CT_PREEMPT_YIELD_DIRECTED	0x04	/* yield - directed */
#define CT_PREEMPT_CEDE			0x08	/* cede */

struct tss {
	uval16 prev_link;
	uval16 _1;
	uval esp0;
	uval16 ss0;
	uval16 _2;
	uval esp1;
	uval16 ss1;
	uval16 _3;
	uval esp2;
	uval16 ss2;
	uval16 _4;
	uval cr3;
	uval eip;
	uval eflags;
	union {
		uval gprs[8];
		struct {
			uval eax;
			uval ecx;
			uval edx;
			uval ebx;
			uval esi;
			uval edi;
			uval esp;
			uval ebp;
		} regs;
	} gprs;
	union {
		/* these are 16 bits + 16 bits padding */
		uval32 srs[6];
		struct {
			uval32 es;
			uval32 cs;
			uval32 ss;
			uval32 ds;
			uval32 fs;
			uval32 gs;
		} regs;
	} srs;
	uval16 ldt_segment;
	uval16 _5;
	uval16 _6;
	uval16 iomap_base_addr;
	uval threadp;
	uval drs[8];		/* debug registers */
};

struct dtr {
	uval16 size;
	uval32 base __attribute__ ((packed));
};

struct cpu_thread {
	uval64 eyecatch;
	unsigned char preempt;
	uval8 thr_num;
	uval8 cpu_num;
	lock_t lock;
	struct cpu *cpu;
	uval16 local_aipc_idx;	/* id of local AIPC buf */
	struct xir_queue xirq;
	uval32 lastirq;

	uval reg_cr0;
	uval reg_cr2;
	uval reg_cr4;
	uval64 reg_tsc;

	/* pointer into the mailbox within the OS addr space */
	struct mailbox *mbox;

	/* current page table directory */
	union pgframe *pgd;

	/* flushed page table directory */
	union pgframe *flushed;

	/* if set, iret calls this function before returning */
	void (*cb) (struct cpu_thread *);

	/*
	 * FPU/MMX/SSE state-save area sufficient for either fxsave/fxrstor
	 * or fsave/frstor.  fxsave and fxrstor are architected to take a
	 * 512-byte storage area, aligned on a 16-byte boundary.
	 */
	uval8 fp_mmx_sse[512] __attribute__ ((aligned(16)));

	/* the tss actually installed on the cpu */
	struct tss tss;

	/* the interrupt descriptor table actually installed on the cpu */
	struct descriptor idt[NR_IDT_ENTRIES] __attribute__ ((aligned(32)));
	struct idt_descriptor idt_info[NR_IDT_ENTRIES];
	struct dtr idtr;

	/* the global descriptor table actually installed on the cpu */
	struct descriptor gdt[GDT_ENTRY_COUNT] __attribute__ ((aligned(8)));
	struct gdt_entry_info gdt_info[GDT_ENTRY_COUNT];
	struct dtr gdtr;
};

extern void deliver_intr_exception(struct cpu_thread *);

/*
 * Check for any pending interrupts and deliver them
 */
static inline void
check_intr_delivery(struct cpu_thread *thread)
{
	struct mailbox *mbox = thread->mbox;

	if (mbox && mbox->irq_pending & ~mbox->irq_masked)
		deliver_intr_exception(thread);
}

#endif /* __HYPE_X86_CPU_THREAD_H__ */
