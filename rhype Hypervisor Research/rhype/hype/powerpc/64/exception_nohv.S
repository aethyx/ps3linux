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
#include <config.h>
#include <hv_asm.h>
#include <lpar.h>
#include <os.h>
#include <asm_defs.h>
#include <hypervisor.h>
#include <xh.h>
#include <thread_control_area.h>
#include <vregs.h>
		
.macro SAVE_SPECIALS_TO_BUFFER oflr, ofctr, ofxer, scratch, buffer
	/* Save registers to buffer */
	/* cr already saved */
	mflr	\scratch
	STR	\scratch, \oflr(\buffer)
	mfctr	\scratch
	STR	\scratch, \ofctr(\buffer)
	mfxer	\scratch
	stw	\scratch, \ofxer(\buffer)
.endm

.macro SAVE_SPECIALS_TO_VREGS scratch, vregs
	SAVE_SPECIALS_TO_BUFFER VS_LR, VS_CTR, VS_XER, \scratch, \vregs
.endm

.macro SAVE_SPECIALS_TO_THREAD scratch, thread
	SAVE_SPECIALS_TO_BUFFER CT_LR, CT_CTR, CT_XER, \scratch, \thread
.endm

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
	
.macro SAVE_R11_R13_TO_BUFFER scratch, offset, reg 
	mfsprg1 \scratch  /* r11 and scratch are saved in sprg 1, 2 */
	STR	\scratch, (\offset + REG_WIDTH * (r11))(\reg)
	mfsprg2 \scratch
	STR	\scratch, (\offset + REG_WIDTH * (r12))(\reg)
	mfsprg3 \scratch
	STR	\scratch, (\offset + REG_WIDTH * (r13))(\reg)
.endm


	
			
.macro  RESTORE_SPECIALS_FROM_VREGS scratch, vregs
	LDR	\scratch, VS_LR(\vregs)
	mtlr	\scratch	
	LDR	\scratch, VS_CTR(\vregs)
	mtctr	\scratch	
	lwz	\scratch, VS_XER(\vregs)
	mtxer	\scratch
	lwz	\scratch, VS_CR(\vregs)
	mtcr	\scratch
.endm


/* this will make the following things happen:
 * r13 = address of the TCA
 * r11 = address of the thread's vexc_save area
 * r12 = scratch
 *
 * sprg3 = original r13
 * sprg2 = original r12
 * sprg1 = original r11
 * tca->active_thread->reg_cr = original cr
 *
 * If thread is in kernel mode, will go to "exception_cont"
 * If thread is in user mode, will go to "reflect_exception"
 */

.macro HYPE_COMMON_PROLOG vec, gdb_breakout
	mtspr	SPRN_SPRG3, r13
	mtspr	SPRN_SPRG2, r12
	mtspr	SPRN_SPRG1, r11
	mfspr	r12, SPRN_SPRG0
	mfcr	r13	/* andi. will modify cr */
	mfsrr0	r11
	std	r11, \vec + 8 (r0)
	mfsrr1	r11
	std	r11, \vec + 16 (r0)
.if \gdb_breakout
#ifdef USE_GDB_STUB
	mfsrr1	r11
	andi.	r11, r11, MSR_PR
	bne	1f
	li	r11, \vec
	ba	ex_gdb_save
1:
#endif /* USE_GDB_STUB */
.endif
	LDR	r11, TCA_VREGS(r12)

	LDR	r12, V_ACTIVE_AREA(r11)
	addi	r12, r12, 1
	andi.	r12, r12, NUM_EXC_SAVES - 1
	mulli	r12, r12, VS_SIZE
	add	r11, r12, r11
	/* r11 = &tca->vregs[(vregs->active_vsave + 1) & (NUM_EXC_SAVES-1)] */

	stw	r13, VS_CR(r11)

	li	r13, \vec
	stw	r13, VS_EXC_NUM(r11)

	mfspr	r13, SPRN_SPRG0	/* get r13 for good */
	STR	r11, TCA_SAVE_AREA(r13)

	mfsrr0	r12
	STR	r12, TCA_SRR0(r13)
/*	STR	r12, VS_SRR0(r11)*/
	mfsrr1	r12
	STR	r12, TCA_SRR1(r13)
/*	STR	r12, VS_SRR1(r11)*/
.endm

	
		
.macro HYPE_CONDITIONAL_PROLOG vec, condition, uspace_target, kspace_target,gdb
	HYPE_COMMON_PROLOG \vec, \gdb
	
	lwz	r12, TCA_VSTATE(r13)
	andi.	r12, r12, \condition
	beq	1f
	ba	\kspace_target		
1:
	ba	\uspace_target
.endm

.macro HYPE_PROLOG vec, target, gdb_breakout
	HYPE_COMMON_PROLOG \vec, \gdb_breakout
	ba	\target		
.endm

	
.macro SAVE_VOLATILES_AND_REFLECT_EXCEPTION exnum
	SAVE_SPECIALS_TO_VREGS r12, r11
	SAVE_GPRS_TO_BUFFER r0, r10, VS_GPRS, r11
	SAVE_R11_R13_TO_BUFFER r12, VS_GPRS, r11

	LDR	r1, TCA_HYPE_STACK(r13)	/* get stack pointer */

	LOADADDR(r5, insert_exception)
	li	r4, \exnum
	LDR	r3, TCA_CPU_THREAD(r13)
	CALL_CFUNC(r5)

	NONSTD_RESTORE_BREAKOUT r4, r11

	LDR	r12, TCA_SAVE_AREA(r13)
	b	partial_restore_save_area
.endm

/*
 * Hypervisor exception handling code.  Copied down to physical address
 * zero by h_init().
 */
	TEXT_ENTRY(hype_vec)
	.globl hype_vecend

	.space 0xc0-(DOT-hype_vec)
r3save:					# need something else for SMP...
	.space 0xc8-(DOT-hype_vec)
os_save:
	.space 0x8

	. = 0x100	# System Reset
C_TEXT_ENTRY(sys_reset)
### FIXME:	Need code here to detect if we are here for thread
###		awake or a true reset.
ex_sysreset:
        mfspr r12, SPRN_PIR
	LOADADDR(r13, tca_table)
	LDR	r13, 0(r13)
	sldi	r12,r12,3	# log2 of 8 for a uval in 64 bit mode
	add	r3, r12, r13		
	mr	r13, r3
	LDR r2,TCA_HYPE_TOC(r13)
	LDR r1,TCA_HYPE_STACK(r13)
	ba .thread_init
        .long 0	

	. = 0x200	# Machine Check
	ba	ex_machcheck

	. = 0x300
	ba	ex_dsi

	. = 0x380
	ba	ex_data_slb

	. = 0x400
	ba	ex_isi

	. = 0x480
	ba	ex_inst_slb

	. = 0x500
	ba	ex_interrupt

	. = 0x600
	ba	ex_alignment

	. = 0x700
	ba	ex_program

	. = 0x800
	ba	ex_float

	. = 0x900
	ba	ex_dec

	. = 0xc00
	ba	ex_syscall
	
	. = 0xd00
	ba	ex_trace
	
	. = 0xe00
	ba	ex_fp
			
ex_machcheck:
	HYPE_PROLOG 0x200, force_gdb_reflect, 0
	
ex_dsi:
	HYPE_PROLOG 0x300, kspace_dsi, 0

ex_data_slb:
	HYPE_PROLOG 0x380, kspace_slb, 0

ex_isi:
	HYPE_PROLOG 0x400, kspace_isi, 0

ex_inst_slb:
	HYPE_PROLOG 0x480, kspace_inst_slb, 0

ex_interrupt:
	HYPE_PROLOG 0x500, pre_handle_external, 0

ex_alignment:
	HYPE_PROLOG 0x600, kspace_prog_ex, 1
		
ex_program:
	HYPE_PROLOG 0x700, kspace_prog_ex, 1

ex_float:
	HYPE_PROLOG 0x800, float_reflect, 0

ex_dec:
	HYPE_CONDITIONAL_PROLOG 0x900, VSTATE_ACTIVE_DEC, fire_hdec, fire_dec, 0

ex_syscall:
	HYPE_CONDITIONAL_PROLOG 0xc00, VSTATE_KERN_MODE, \
				uspace_syscall, kspace_syscall, 0
	
ex_trace:
	HYPE_PROLOG 0xd00, force_gdb_reflect, 0
	
ex_fp:
	HYPE_PROLOG 0xe00, force_gdb_reflect, 0

	.align 3
hype_vecend:			# but some stuff here so we see the next symbol
	.long 0xdeadbeef
	.long 0xdeadbeef

kspace_noimp:
uspace_noimp:
	tw	31, 0, 0
	b	uspace_noimp

uspace_isi:	
uspace_inst_slb:
uspace_dsi:
uspace_slb:	
	tw	31, 0, 0
	b	uspace_noimp
	

.macro NONSTD_RESTORE_BREAKOUT ra, rb
	LDR	\ra, TCA_CPU_THREAD(r13)
	lbz	\rb, CT_PREEMPT(\ra)
	CMPI	\rb, 0 /* should we preempt? */
	bnel	save_all
	LDR	\ra, TCA_RESTORE_FN(r13)
	cmpdi	\ra, 0
	bne-	alternate_restore
.endm
	
.macro MEM_EXCEPTION_HANDLER type, func
	SAVE_SPECIALS_TO_VREGS r12, r11
	SAVE_GPRS_TO_BUFFER r0, r10, VS_GPRS, r11
	SAVE_R11_R13_TO_BUFFER r12, VS_GPRS, r11
	LDR	r1,TCA_HYPE_STACK(r13) 	/* get stack pointer */
	li	r4, \type
	LDR	r3, TCA_CPU_THREAD(r13)
	mr	r5, r11
	LOADADDR(r6, \func)
	CALL_CFUNC(r6)

	NONSTD_RESTORE_BREAKOUT r4, r11

	LDR	r12, TCA_SAVE_AREA(r13)
	/* handler function returns value of r3 */
	b	partial_restore_save_area
.endm

kspace_slb:
	MEM_EXCEPTION_HANDLER 1, xh_kern_slb

kspace_dsi:
	MEM_EXCEPTION_HANDLER 1, xh_kern_pgflt

kspace_inst_slb:
	MEM_EXCEPTION_HANDLER 0, xh_kern_slb

kspace_isi:
	MEM_EXCEPTION_HANDLER 0, xh_kern_pgflt
				
kspace_prog_ex:
	/* This could be a user-space trap */
	lwz	r12, TCA_VSTATE(r13)
	andi.	r12, r12, VSTATE_KERN_MODE
	beq	uspace_prog_ex

	SAVE_SPECIALS_TO_VREGS r12, r11
	SAVE_GPRS_TO_BUFFER r0, r10, VS_GPRS, r11
	SAVE_R11_R13_TO_BUFFER r12, VS_GPRS, r11

	/* Save rest of gprs to thread struct */
	LDR	r11, TCA_CPU_THREAD(r13)
	SAVE_GPRS_TO_BUFFER r14, r31, CT_GPRS, r11
	
	LDR	r1, TCA_HYPE_STACK(r13) /* get stack pointer */
	ld	r4, TCA_SRR0(r13)
	mr	r3, r11
	LOADADDR(r5, xh_kern_prog_ex)
	CALL_CFUNC(r5)
	LDR	r11, TCA_CPU_THREAD(r13)
	RESTORE_GPRS_FROM_BUFFER r14, r31, CT_GPRS, r11

	NONSTD_RESTORE_BREAKOUT r4, r11
	LDR	r12, TCA_SAVE_AREA(r13)

	cmpdi	r3, 0
	bge	partial_restore_save_area	

	RESTORE_GPRS_FROM_BUFFER r3, r3, VS_GPRS, r12
	b	partial_restore_save_area

pre_handle_external:
	SAVE_SPECIALS_TO_VREGS r12, r11
	SAVE_GPRS_TO_BUFFER r0, r10, VS_GPRS, r11
	SAVE_R11_R13_TO_BUFFER r12, VS_GPRS, r11

	LDR	r3, TCA_CPU_THREAD(r13)
	LDR	r1, TCA_HYPE_STACK(r13) /* get stack pointer */
	LOADADDR(r4, handle_external)
	CALL_CFUNC(r4)
	
	NONSTD_RESTORE_BREAKOUT r4, r11
	LDR	r12, TCA_SAVE_AREA(r13)

	RESTORE_GPRS_FROM_BUFFER r3, r3, VS_GPRS, r12
	b	partial_restore_save_area
	
force_gdb_reflect:	
uspace_prog_ex:
	SAVE_VOLATILES_AND_REFLECT_EXCEPTION EXC_V_DEBUG
	
uspace_syscall:
	SAVE_VOLATILES_AND_REFLECT_EXCEPTION EXC_V_SYSCALL

float_reflect:
	SAVE_VOLATILES_AND_REFLECT_EXCEPTION EXC_V_FP

external_reflect:
	SAVE_VOLATILES_AND_REFLECT_EXCEPTION EXC_V_EXT

	
			
kspace_syscall:		
	/* For syscalls from kernel to kernel, we have to reflect */
	cmpdi	r0, -1
	bne	uspace_syscall

	SAVE_SPECIALS_TO_VREGS r12, r11
	SAVE_GPRS_TO_BUFFER r0, r2, VS_GPRS, r11
	SAVE_R11_R13_TO_BUFFER r12, VS_GPRS, r11

	LDR	r1, TCA_HYPE_STACK(r13)	/* get stack pointer */

	/* The ABI wants the parameter on R11,R12 to be on the stack */
	subi	r1, r1, 16
	mfsprg1 r12  /* r11, r12, r13 are saved in sprg 1, 2 3,*/
	STR	r12, 112(r1)
	mfsprg2 r12
	STR	r12, 120(r1)
	
	LOADADDR(r12, xh_syscall)
	CALL_CFUNC(r12)		

	NONSTD_RESTORE_BREAKOUT r4, r11

	LDR	r12, TCA_SAVE_AREA(r13)

partial_restore_save_area:	
	/* r3 is restored (is hcall return val)
	 * r12 is pointer to vexc_save struct
	 */
	RESTORE_SPECIALS_FROM_VREGS r0, r12
	RESTORE_GPRS_FROM_BUFFER r0, r2, VS_GPRS, r12
	RESTORE_GPRS_FROM_BUFFER r4, r10, VS_GPRS, r12
#if 0
	LOADADDR(r11, post_rfid)
	mtsrr0	r11
	LOADCONST(r11, MSR_SF|MSR_ME)
	mtsrr1	r11
	rfid
post_rfid:	
#endif
	LDR	r11,	TCA_SRR0(r13)
	mtsrr0	r11
	LDR	r11,	TCA_SRR1(r13)
	mtsrr1	r11
	RESTORE_GPRS_FROM_BUFFER r13, r13, VS_GPRS, r12
	RESTORE_GPRS_FROM_BUFFER r11, r12, VS_GPRS, r12
	HRFID

	/* This is used to insert an alternate restoration path, either
	 * delivery of an exception or preemption */
alternate_restore:
	li	r6, 0
	STR	r6, TCA_RESTORE_FN(r13)  /* Clear the restore_fn field */
	LDR	r3, TCA_CPU_THREAD(r13)
	CALL_CFUNC(r4)
	

C_TEXT_ENTRY(insert_ext_exception)
	LDR	r1,TCA_HYPE_STACK(r13) 	/* get stack pointer */
	LDR	r3, TCA_CPU_THREAD(r13)

	LOADADDR(r5, xh_ext)
	CALL_CFUNC(r5)		/* returns idx of save area to restore */
	/* r3 contains vsave idx */

	/* Just set up srr's and rfid... */
	LDR	r11,	TCA_SRR0(r13)
	mtsrr0	r11
	LDR	r11,	TCA_SRR1(r13)
	mtsrr1	r11
	rfid

		
fire_dec:
	SAVE_SPECIALS_TO_VREGS r12, r11
	SAVE_GPRS_TO_BUFFER r0, r10, VS_GPRS, r11
	SAVE_R11_R13_TO_BUFFER r12, VS_GPRS, r11

	/* if a dec exception is being forced from HV (e.g. mtmsrd 
	 * unsets MSR_EE and a dec was pending) we end up here, via
	 * the alternate_restore path.
         */ 
C_TEXT_ENTRY(insert_dec_exception)
	LDR	r1,TCA_HYPE_STACK(r13) 	/* get stack pointer */
	LDR	r3, TCA_CPU_THREAD(r13)

	LOADADDR(r5, xh_dec)
	CALL_CFUNC(r5)		/* returns idx of save area to restore */
	/* r3 contains vsave idx */
	cmpdi	r3, 0
	blt-	dec_restore

	/* Just set up srr's and rfid... */
	LDR	r11,	TCA_SRR0(r13)
	mtsrr0	r11
	LDR	r11,	TCA_SRR1(r13)
	mtsrr1	r11
	rfid

dec_restore:
	LDR	r12, TCA_SAVE_AREA(r13)
	RESTORE_GPRS_FROM_BUFFER r3, r3, VS_GPRS, r12
	b partial_restore_save_area
	
			
fire_hdec:
	LDR	r12, TCA_CPU_THREAD(r13)
	SAVE_GPRS_TO_BUFFER r0, r10, CT_GPRS, r12
	SAVE_SPECIALS_TO_THREAD r10, r12
	SAVE_R11_R13_TO_BUFFER r10, CT_GPRS, r12
	SAVE_GPRS_TO_BUFFER r14, r31, CT_GPRS, r12

	/* cr is saved very early on into vregs */
	lwz	r4, VS_CR(r11)
	LDR	r5, TCA_SRR0(r13)
	LDR	r6, TCA_SRR1(r13)

	stw	r4, CT_CR(r12)
	STR	r5, CT_HSRR0(r12)
	STR	r6, CT_HSRR1(r12)

	li	r4, 1
	mr	r3, r12
	LDR	r1, TCA_HYPE_STACK(r13) 	/* get stack pointer */

	LOADADDR(r5, preempt_thread)	
	CALL_CFUNC(r5)		

	/* We don't come back here */
	b .
			
save_all:	
	/* preempt this OS */
	SAVE_GPRS_TO_BUFFER r17, r31, CT_GPRS, r14
	mfsrr0 r15
	STR r15,CT_HSRR0(r14)
	mfsrr1 r15
	STR r15,CT_HSRR1(r14)
        mr r3, r14
	li r4,0	
	bl C_TEXT(preempt_thread)



#ifdef USE_GDB_STUB
ex_gdb_save:
	
	/*
	 * at this point:
	 * r13 = saved cr
	 * r11 = exception vector
	 * r12 = scratch
	 *
	 * sprg3 = original r13
	 * sprg2 = original r12
	 * sprg1 = original r11
	 * tca->active_thread->reg_cr = original cr
	 */
	/* Save non-gpr volatiles */
	LOADADDR(r12, gdb_currdbg)
	LDR	r12, 0(r12)
	subi	r12, r12, GDB_CPU_STATE_SIZE

	SAVE_GPRS_TO_BUFFER r0, r10, GDB_GPR0, r12
	SAVE_R11_R13_TO_BUFFER r9, 0, r12
	SAVE_SPECIALS_TO_BUFFER GDB_LR, GDB_CTR, GDB_XER, r9, r12
	SAVE_GPRS_TO_BUFFER r14, r31, GDB_GPR0, r12

	stw	r13, GDB_CR(r12)	/* 32b reg */
	mfsprg0	r13

	mr	r4, r11
	
	mfsrr0	r8
	STR	r8, GDB_PC(r12)	

	mfsrr1	r7
	STR	r7, GDB_MSR(r12)	


	mfspr	r8, SPRN_HSRR0
	STR	r8, GDB_HSRR0(r12)

	mfspr	r8, SPRN_HSRR1
	STR	r8, GDB_HSRR1(r12)
		
	mfdar	r6
	STR	r6, GDB_DAR(r12)	

	mfdsisr	r5
	STR	r5, GDB_DSISR(r12)	

	mfspr	r9, SPRN_DEC
	STR	r9, GDB_HDEC(r12)	

	mr	r3, r12
	subi	r1, r12, MIN_FRAME_SZ

	LOADADDR(r5,enter_gdb)
	CALL_CFUNC(r5)
	nop

	ld	r10, GDB_MSR(r3)
	ld	r11, GDB_PC(r3)
	lwz	r12, GDB_CR(r3)
	lwz	r13, GDB_XER(r3)
	ld	r14, GDB_CTR(r3)
	ld	r15, GDB_LR(r3)
	ld	r2, GDB_HDEC(r3)	
	
	mtsrr1	r10
	mtsrr0	r11
	mtcr	r12
	mtxer	r13
	mtctr	r14
	mtlr	r15

	ld	r10, GDB_DAR(r3)
	ld	r11, GDB_DSISR(r3)
	mtdar	r10
	mtdsisr r11
	
	RESTORE_GPRS_FROM_BUFFER r0, r1, 0, r3
	RESTORE_GPRS_FROM_BUFFER r4, r31, 0, r3

	addi	r2, r2, 32
	mtspr	SPRN_DEC, r2
	RESTORE_GPRS_FROM_BUFFER r2, r3, 0, r3
	rfid

	.long 0


GLBL_LABEL(trap_instruction)
	tw	31,0,0
#endif
