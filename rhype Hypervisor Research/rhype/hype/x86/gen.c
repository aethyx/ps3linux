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
#include <asm.h>
#include <hv_regs.h>
#include <thread_control_area.h>
#include <util.h>
#include <gdbstub.h>

int
main(void)
{
	DECLARE(CR0, offsetof(struct cpu_thread, reg_cr0));

	/* CR1 reserved */
	DECLARE(CR2, offsetof(struct cpu_thread, reg_cr2));

	/* CR2 in struct tss */
	DECLARE(CR4, offsetof(struct cpu_thread, reg_cr4));

	DECLARE(IDTR, offsetof(struct cpu_thread, idtr));
	DECLARE(GDTR, offsetof(struct cpu_thread, gdtr));

	DECLARE(CR3,
		offsetof(struct cpu_thread, tss) + offsetof(struct tss, cr3));
	DECLARE(EIP,
		offsetof(struct cpu_thread, tss) + offsetof(struct tss, eip));
	DECLARE(EFLAGS,
		offsetof(struct cpu_thread, tss) + offsetof(struct tss,
							    eflags));

	DECLARE(CS,
		offsetof(struct cpu_thread, tss) + offsetof(struct tss,
							    srs.regs.cs));
	DECLARE(DS,
		offsetof(struct cpu_thread, tss) + offsetof(struct tss,
							    srs.regs.ds));
	DECLARE(SS,
		offsetof(struct cpu_thread, tss) + offsetof(struct tss,
							    srs.regs.ss));
	DECLARE(ES,
		offsetof(struct cpu_thread, tss) + offsetof(struct tss,
							    srs.regs.es));
	DECLARE(FS,
		offsetof(struct cpu_thread, tss) + offsetof(struct tss,
							    srs.regs.fs));
	DECLARE(GS,
		offsetof(struct cpu_thread, tss) + offsetof(struct tss,
							    srs.regs.gs));

	DECLARE(EAX,
		offsetof(struct cpu_thread, tss) + offsetof(struct tss,
							    gprs.regs.eax));
	DECLARE(EBX,
		offsetof(struct cpu_thread, tss) + offsetof(struct tss,
							    gprs.regs.ebx));
	DECLARE(ECX,
		offsetof(struct cpu_thread, tss) + offsetof(struct tss,
							    gprs.regs.ecx));
	DECLARE(EDX,
		offsetof(struct cpu_thread, tss) + offsetof(struct tss,
							    gprs.regs.edx));
	DECLARE(ESI,
		offsetof(struct cpu_thread, tss) + offsetof(struct tss,
							    gprs.regs.esi));
	DECLARE(EDI,
		offsetof(struct cpu_thread, tss) + offsetof(struct tss,
							    gprs.regs.edi));
	DECLARE(ESP,
		offsetof(struct cpu_thread, tss) + offsetof(struct tss,
							    gprs.regs.esp));
	DECLARE(EBP,
		offsetof(struct cpu_thread, tss) + offsetof(struct tss,
							    gprs.regs.ebp));
	DECLARE(DR0,
		offsetof(struct cpu_thread, tss) +
		offsetof(struct tss, drs[0]));
	DECLARE(DR1,
		offsetof(struct cpu_thread, tss) +
		offsetof(struct tss, drs[1]));
	DECLARE(DR2,
		offsetof(struct cpu_thread, tss) +
		offsetof(struct tss, drs[2]));
	DECLARE(DR3,
		offsetof(struct cpu_thread, tss) +
		offsetof(struct tss, drs[3]));
	DECLARE(DR6,
		offsetof(struct cpu_thread, tss) +
		offsetof(struct tss, drs[6]));
	DECLARE(DR7,
		offsetof(struct cpu_thread, tss) +
		offsetof(struct tss, drs[7]));

	DECLARE(PREEMPT_THREAD, offsetof(struct cpu_thread, preempt));

	DECLARE(FP_MMX_SSE, offsetof(struct cpu_thread, fp_mmx_sse));

#ifdef USE_GDB_STUB
	DECLARE(GDB_EAX, offsetof(struct cpu_state, eax));
	DECLARE(GDB_ECX, offsetof(struct cpu_state, ecx));
	DECLARE(GDB_EDX, offsetof(struct cpu_state, edx));
	DECLARE(GDB_EBX, offsetof(struct cpu_state, ebx));
	DECLARE(GDB_ESP, offsetof(struct cpu_state, esp));
	DECLARE(GDB_EBP, offsetof(struct cpu_state, ebp));
	DECLARE(GDB_ESI, offsetof(struct cpu_state, esi));
	DECLARE(GDB_EDI, offsetof(struct cpu_state, edi));
	DECLARE(GDB_EIP, offsetof(struct cpu_state, eip));
	DECLARE(GDB_EFLAGS, offsetof(struct cpu_state, eflags));
	DECLARE(GDB_CS, offsetof(struct cpu_state, cs));
	DECLARE(GDB_SS, offsetof(struct cpu_state, ss));
	DECLARE(GDB_DS, offsetof(struct cpu_state, ds));
	DECLARE(GDB_ES, offsetof(struct cpu_state, es));
	DECLARE(GDB_FS, offsetof(struct cpu_state, fs));
	DECLARE(GDB_GS, offsetof(struct cpu_state, gs));
#endif /* USE_GDB_STUB */

	return 0;
}
