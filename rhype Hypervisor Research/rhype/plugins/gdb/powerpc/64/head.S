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
 * Will get linked first for all test OSes
 */

#include <config.h>
#include <asm.h>
#include <lpar.h>
#include <hypervisor.h>
#include <xh.h>
#include <vregs.h>
#include <asm_defs.h>
	
/*
 * Test code for OS boot and exception vectors.
 */
.macro SAVE_GPRS_TO_BUFFER from, to, offset, reg
	STR	\from, (\offset + REG_WIDTH * \from)(\reg)
.if	\to - \from
	SAVE_GPRS_TO_BUFFER "(\from+1)", \to, \offset, \reg
.endif
.endm

.macro RESTORE_GPRS_FROM_BUFFER from, to, offset, reg
	LDR	\from, \offset + REG_WIDTH * \from(\reg)
.if	\to - \from
	RESTORE_GPRS_FROM_BUFFER "(\from+1)", \to, \offset, \reg
.endif
.endm

.macro COPY_GPRS from, to, src_off, src, dest_off, dest, reg
	LDR	\reg, ((REG_WIDTH * \from) + \src_off)(\src)
	STR	\reg, ((REG_WIDTH * \from) + \dest_off)(\dest)
.if	\to - \from
	COPY_GPRS "(\from+1)", \to, \src_off, \src, \dest_off, \dest, \reg
.endif
.endm
	
	. = 0x0
	.globl entry_point
entry_point:
	/*
	 * This is here so controller can start at zero.  Later we
	 * will nop this instruction so that if a test OS branches to 0x0
	 * somehow, it will evetually assert
	 */
	mulli	r13, r3, VS_SIZE
	addi	r13, r13, V_EXC_SAVE + VREG_BASE

	LOADADDR(r1, gdb_currdbg)
	LDR	r1, 0(r1)
	
	subi	r1, r1, GDB_CPU_STATE_SIZE

	SAVE_GPRS_TO_BUFFER r14, r31, 0, r1

	COPY_GPRS r0, r14, VS_GPRS, r13, 0, r1, r4

	mr	r14, r3
	
	ld	r2, VS_LR(r13)
	STR	r2, GDB_LR(r1)

	ld	r2, VS_CTR(r13)
	STR	r2, GDB_CTR(r1)	

	lwz	r2, VS_XER(r13)
	stw	r2, GDB_XER(r1)	

	lwz	r2, VS_CR(r13)
	stw	r2, GDB_CR(r1)	

	ld	r2, VS_SRR0(r13)
	STR	r2, GDB_PC(r1)	

	ld	r2, VS_SRR1(r13)
	STR	r2, GDB_MSR(r1)	

	ld	r2, V_DAR + VREG_BASE(r0)
	STR	r2, GDB_DAR(r1)	

	ld	r2, V_DSISR + VREG_BASE(r0)
	STR	r2, GDB_DSISR(r1)

	ld	r2, V_DEC + VREG_BASE(r0)
	STR	r2, GDB_HDEC(r1)
		
	mr	r3, r1
	subi	r1, r1, MIN_FRAME_SZ

	
	lwz	r4, VS_EXC_NUM(r13)

	LOADADDR(r5,enter_gdb)
	CALL_CFUNC(r5)


	ld	r4, GDB_MSR(r3)
	ld	r5, GDB_PC(r3)
	lwz	r6, GDB_CR(r3)
	lwz	r7, GDB_XER(r3)
	ld	r8, GDB_CTR(r3)
	ld	r9, GDB_LR(r3)

	STR	r4, VS_SRR1(r13)
	STR	r5, VS_SRR0(r13)
	stw	r6, VS_CR(r13)
	stw	r7, VS_XER(r13)
	STR	r8, VS_CTR(r13)
	STR	r9, VS_LR(r13)

	ld	r10, GDB_DAR(r3)
	ld	r11, GDB_DSISR(r3)
	STR	r10, V_DAR + VREG_BASE(r0)
	STR	r11, V_DSISR + VREG_BASE(r0)

	mr	r4, r14

	COPY_GPRS r0, r13, 0, r3, VS_GPRS, r13, r5
	RESTORE_GPRS_FROM_BUFFER r14, r31, 0, r3

	ld	r5, GDB_HDEC(r3)
	li	r3, 0x60cc
	li	r0, -1
	sc
	b	.

GLBL_LABEL(trap_instruction)
	tw	31,0,0

	
/*
 * Start of Data Segment
 */
#define STACK_SIZE 0x8000
DATA_ENTRY(3)

	.fill 256, 4, 0xdeadbeef
stackguard:
	.space STACK_SIZE - MIN_FRAME_SZ
stackbottom:
	.space MIN_FRAME_SZ
	.fill 256, 4, 0xdeadbeef
stacktop:	
	.space MIN_FRAME_SZ
