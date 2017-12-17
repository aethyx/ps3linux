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

#define SAVE_REGISTER(rn) \
	STR rn,CT_GPRS+(REG_WIDTH*(rn))(r14)

#define SAVE_GPR(base, rn) STR  rn , (REG_WIDTH * (rn)) (base)
#define SAVE_2GPR(base, rn) SAVE_GPR(base, rn) ;  SAVE_GPR(base,(rn+1))
#define SAVE_4GPR(base, rn) SAVE_2GPR(base, rn) ;  SAVE_2GPR(base, (rn + 2))
#define SAVE_8GPR(base, rn) SAVE_4GPR(base,rn) ;  SAVE_4GPR(base, (rn + 4))
#define SAVE_16GPR(base, rn) SAVE_8GPR(base, rn) ;  SAVE_8GPR(base, (rn + 8))

#define LOAD_GPR(base, rn) LDR  rn , (REG_WIDTH * (rn)) (base)
#define LOAD_2GPR(base, rn) LOAD_GPR(base,rn) ;  LOAD_GPR(base, (rn+1))
#define LOAD_4GPR(base, rn) LOAD_2GPR(base, rn) ;  LOAD_2GPR(base, (rn + 2))
#define LOAD_8GPR(base, rn) LOAD_4GPR(base, rn) ;  LOAD_4GPR(base, (rn + 4))
#define LOAD_16GPR(base,rn) LOAD_8GPR(base, rn) ;  LOAD_8GPR(base, (rn + 8))

/* this will make the following things happen:
 * r13 - address of the TCA
 * r14 - address of the CPU_THREAD
 * r15 - scratch
 * r16 - scratch
 *

	 * original r13-r16 & cr saved in the TCA
 *
 * r13-r16 were chosen for the following reasons:
 *
 * r13 is special, as defined in the ppc64-abi
 * r14-r16 were used since hcalls use r3-r8, and
 * r0-r2 have special meanings later when we call c code
 */

#define HYPE_COMMON_PROLOG(vector) \
        mtspr SPRN_HSPRG1, r13; \
	mfspr r13,SPRN_HSPRG0; \
        STR r14, TCA_CACHED_R14(r13); \
        mfspr r14, SPRN_HSPRG1; \
        STR r14, TCA_CACHED_R13(r13); \
        LDR r14, TCA_CPU_THREAD(r13); \
        STR r15, CT_GPRS+(REG_WIDTH*(r15))(r14); \
        STR r16, CT_GPRS+(REG_WIDTH*(r16))(r14); \
        mfcr r15; \
        stw r15, CT_CR(r14); \
        LDR r15, TCA_CACHED_R13(13); \
        STR r15, CT_GPRS+(REG_WIDTH*(r13))(r14); \
        LDR r15, TCA_CACHED_R14(13); \
        STR r15, CT_GPRS+(REG_WIDTH*(r14))(r14)

#ifndef USE_GDB_STUB
#define TRAP_PRE(vec) b .
#else
	/* Save registers for gdb stub. */
	/* Need to use gdb-stub specific data structures
	 * so as to not corrupt HV data structures, such as those
	 * that store saved register state.  Also, if possible we want to
	 * avoid sharing code paths with mainline code.
	 */
	
	/* Need to save non-gpr volatiles ASAP and get out of here,
	 * since there's not enough space within the exception vector.
	 */
#define TRAP_PRE(vec) \
	mtspr	SPRN_HSPRG1, r1; \
	LOADADDR(r1, gdb_currdbg); \
	LDR	r1, 0(r1); \
	subi	r1, r1, GDB_CPU_STATE_SIZE; \
	SAVE_2GPR(r1, r2); \
	mfctr	r2; \
	STR	r2, GDB_CTR(r1); \
	LOADADDR(r2, ex_gdb_save); \
	mtctr	r2; \
	li	r3, vec; \
	bctr
# endif /* USE_GDB_STUB */

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
ex_machcheck:
	TRAP_PRE(0x200)

	. = 0x300
ex_dsi:
	TRAP_PRE(0x300)
	.long 0

	. = 0x380
ex_data_slb:
	TRAP_PRE(0x380)
        .long 0             # we dont do this

	. = 0x400
ex_isi:
	TRAP_PRE(0x400)
        .long 0             # we dont do this

	. = 0x480
ex_inst_slb:
	TRAP_PRE(0x480)
        .long 0             # we dont do this

	. = 0x500
ex_interrupt:
        /* sets up the TCA, CPU_THREAD, and r15 & r16 as scratch */
        HYPE_COMMON_PROLOG(0x500)
	ba	ext_cont


	. = 0x600
ex_alignment:
	TRAP_PRE(0x600)
	
	. = 0x700
	
ex_program:
	TRAP_PRE(0x700)

	. = 0x800
ex_float:
	TRAP_PRE(0x800)

	. = 0x900
ex_dec:
	TRAP_PRE(0x900)

	. = 0x980
ex_hdec:
        /* sets up the TCA, CPU_THREAD, and r15 & r16 as scratch */
        HYPE_COMMON_PROLOG(0x980)
	ba	hdec_cont

	. = 0xc00
ex_syscall:
	##  FIXME: need to check LPES and do the right thing
#ifdef ALWAYS_LPES_00
	ba ex_syscall_LPES_00
#else
        /* sets up the TCA, CPU_THREAD, and r15 & r16 as scratch */
        HYPE_COMMON_PROLOG(0xc00)
	ba ex_syscall_LPES_x1
# endif
	
	. = 0xd00
ex_trace:
	TRAP_PRE(0xd00)

	. = 0xe00
ex_fp:
	TRAP_PRE(0xe00)

	.align 3
hype_vecend:			# but some stuff here so we see the next symbol
	.long 0xdeadbeef
	.long 0xdeadbeef

ext_cont:
	SAVE_SPECIAL_VOLATILES(r15) /* LR CTR XER */
  	SAVE_REGISTER(r0)
        SAVE_REGISTER(r1)
        SAVE_REGISTER(r2)
        SAVE_REGISTER(r3)
        SAVE_REGISTER(r4)
        SAVE_REGISTER(r5)
        SAVE_REGISTER(r6)
        SAVE_REGISTER(r7)
        SAVE_REGISTER(r8)
        SAVE_REGISTER(r9)
        SAVE_REGISTER(r10)
        SAVE_REGISTER(r11)
        SAVE_REGISTER(r12)	
	LDR r2,TCA_HYPE_TOC(r13)
	LDR r1,TCA_HYPE_STACK(r13)
	mr r3, r14
	bl C_TEXT(handle_external)
	lbz r15,CT_PREEMPT(r14)
	CMPI r15,0  /* should we preempt? */
	beq ext_rfi	
	/* preempt this OS */
        SAVE_REGISTER(r17)
        SAVE_REGISTER(r18)
        SAVE_REGISTER(r19)
        SAVE_REGISTER(r20)
        SAVE_REGISTER(r21)
        SAVE_REGISTER(r22)
        SAVE_REGISTER(r23)
        SAVE_REGISTER(r24)
        SAVE_REGISTER(r25)
        SAVE_REGISTER(r26)
        SAVE_REGISTER(r27)
        SAVE_REGISTER(r28)
        SAVE_REGISTER(r29)
        SAVE_REGISTER(r30)
        SAVE_REGISTER(r31)
	mfsrr0 r15
	STR r15,CT_SRR0(r14)
	mfsrr1 r15
	STR r15,CT_SRR1(r14)
	mfspr r15,SPRN_HSRR0
	STR r15,CT_HSRR0(r14)
	mfspr r15,SPRN_HSRR1
	STR r15,CT_HSRR1(r14)
        mr r3, r14
	li r4,0	
	bl C_TEXT(preempt_thread)
ext_rfi: /* resume this OS */
	RESTORE_SPECIAL_VOLATILES(r15) /* LR CTR XER */
        lwz r15, CT_CR(r14) 
        mtcr r15 	
	RESTORE_REGISTER(r0)
	RESTORE_REGISTER(r1)
	RESTORE_REGISTER(r2)
	RESTORE_REGISTER(r3)
	RESTORE_REGISTER(r4)
	RESTORE_REGISTER(r5)
	RESTORE_REGISTER(r6)
	RESTORE_REGISTER(r7)
	RESTORE_REGISTER(r8)
	RESTORE_REGISTER(r9)
	RESTORE_REGISTER(r10)
	RESTORE_REGISTER(r11)
	RESTORE_REGISTER(r12)
	RESTORE_REGISTER(r15)
	RESTORE_REGISTER(r16)
	RESTORE_REGISTER(r13)
	RESTORE_REGISTER(r14)
	HRFID		

hdec_cont:
	mfsrr0 r15
	STR r15,CT_SRR0(r14)
	mfsrr1 r15
	STR r15,CT_SRR1(r14)
	SAVE_SPECIAL_VOLATILES(r15) /* LR CTR XER */
        SAVE_REGISTER(r0)
        SAVE_REGISTER(r1)
        SAVE_REGISTER(r2)
        SAVE_REGISTER(r3)
        SAVE_REGISTER(r4)
        SAVE_REGISTER(r5)
        SAVE_REGISTER(r6)
        SAVE_REGISTER(r7)
        SAVE_REGISTER(r8)
        SAVE_REGISTER(r9)
        SAVE_REGISTER(r10)
        SAVE_REGISTER(r11)
        SAVE_REGISTER(r12)
	/* 13-16 already saved */
        SAVE_REGISTER(r17)
        SAVE_REGISTER(r18)
        SAVE_REGISTER(r19)
        SAVE_REGISTER(r20)
        SAVE_REGISTER(r21)
        SAVE_REGISTER(r22)
        SAVE_REGISTER(r23)
        SAVE_REGISTER(r24)
        SAVE_REGISTER(r25)
        SAVE_REGISTER(r26)
        SAVE_REGISTER(r27)
        SAVE_REGISTER(r28)
        SAVE_REGISTER(r29)
        SAVE_REGISTER(r30)
        SAVE_REGISTER(r31)
	mfspr r15,SPRN_HSRR0
	STR r15,CT_HSRR0(r14)
	mfspr r15,SPRN_HSRR1
	STR r15,CT_HSRR1(r14)
        mr r3, r14
	LDR r2,TCA_HYPE_TOC(r13)
	LDR r1,TCA_HYPE_STACK(r13)
	li r4,1	
	bl C_TEXT(preempt_thread)
	/* if we return, return to the same OS */
	RESTORE_SPECIAL_VOLATILES(r15) /* LR CTR XER */
        lwz r15, CT_CR(r14) 
        mtcr r15 	
	RESTORE_REGISTER(r0)
	RESTORE_REGISTER(r1)
	RESTORE_REGISTER(r2)
	RESTORE_REGISTER(r3)
	RESTORE_REGISTER(r4)
	RESTORE_REGISTER(r5)
	RESTORE_REGISTER(r6)
	RESTORE_REGISTER(r7)
	RESTORE_REGISTER(r8)
	RESTORE_REGISTER(r9)
	RESTORE_REGISTER(r10)
	RESTORE_REGISTER(r11)
	RESTORE_REGISTER(r12)
	RESTORE_REGISTER(r15)
	RESTORE_REGISTER(r16)
	RESTORE_REGISTER(r13)
	RESTORE_REGISTER(r14)
	HRFID	

	
ex_syscall_LPES_00:
	/* FIXME: this doe not work any more
	 * syscall interrupt For right now, forward user-mode sc to
         * OS, along with kernel-mode sc with r0<0.
	 */
	b 0x700
	.long 0

ex_syscall_LPES_x1:	
	/* r0 = offset into appropriate hcall_vec */
	cmpi cr0,r3,H_YIELD
        bne   nosavevol
        SAVE_REGISTER(r0)
	/* Lets figure out why we are saving volatiles? */
        SAVE_REGISTER(r4)
        SAVE_REGISTER(r5)
        SAVE_REGISTER(r6)
        SAVE_REGISTER(r7)
        SAVE_REGISTER(r8)
        SAVE_REGISTER(r9)
        SAVE_REGISTER(r10)
        SAVE_REGISTER(r11)
        SAVE_REGISTER(r12)
nosavevol:
	mr r0,r3
	RLICR r0,r0,HCALL_VEC_IDX_SHIFT,HCALL_VEC_MASK_NUM
        LDR r12,TCA_LAST_HCALL(r13)
        addi r12,r12,1
	cmpi cr0,r12,32
        blt   small  
big:    li r12,0
small:  STR r12,TCA_LAST_HCALL(r13)
        RLICR r12,r12,3,60
        add r12,r13,r12
        addi r12,r12,TCA_LAST_FEW
        STR r3,0(r12)
	LDR r12,TCA_CACHED_HCA(r13)
        LDR r12,HCA_HCALL_VEC_LEN(r12)
	CMPL r0,r12	# consider masking down to table size to save cycles
	LDR r12,TCA_CACHED_HCA(r13)
        LDR r12,HCA_HCALL_VEC(r12)
	bge not_vec0
	
call_vec:
	SAVE_REGISTER(r1)
        SAVE_REGISTER(r2)
	mflr r15 
	STR r15, CT_LR(r14)
	mfctr r15
	STR r15, CT_CTR(r14)
	
	/* 
	 * Now that we support GDB and other instrumentation based on
	 * the synchronous interrupts we need to save and restore SRR0/1
	 * We save into HSRR0 in case we preempt and later wish to deliver
	 * an external using hrfid.
	 */
	mfsrr0 r15
	STR r15, CT_HSRR0(r14)
	mfsrr1 r15
	STR r15, CT_HSRR1(r14)
	
        add r12,r12,r0
	LDR r12,0(r12)
	CMPI r12,0
	beq bad_hcall

	LDR r1,TCA_HYPE_STACK(r13)		# Get stack pointer
	LDR r2,TCA_HYPE_TOC(r13)
	mr r3,r14

	/* The ABI wants the parameter on R11 to be on the stack */
	STR r11, 112(r1)
	CALL_CFUNC(r12)

return_preempt:
	lbz r15,CT_PREEMPT(r14)
	CMPI r15,0

	SAVE_REGISTER(r3)
	/* condition absolute branches displacement too small */
	beq return_no_preempt

        SAVE_REGISTER(r17)
        SAVE_REGISTER(r18)
        SAVE_REGISTER(r19)
        SAVE_REGISTER(r20)
        SAVE_REGISTER(r21)
        SAVE_REGISTER(r22)
        SAVE_REGISTER(r23)
        SAVE_REGISTER(r24)
        SAVE_REGISTER(r25)
        SAVE_REGISTER(r26)
        SAVE_REGISTER(r27)
        SAVE_REGISTER(r28)
        SAVE_REGISTER(r29)
        SAVE_REGISTER(r30)
        SAVE_REGISTER(r31)
        mr r3, r14
	li r4,0
	bl C_TEXT(preempt_thread)

return_no_preempt:
	bl C_TEXT(rcu_check)
	RESTORE_REGISTER(r3)
	/*
	 * No preemption, this is the fast path return from hcall.
	 * NOTE: we restore SRR0/1 from CT_HSRR0/1
	 */
	LDR r15,CT_LR(r14)
	mtlr r15
	LDR r15,CT_CTR(r14)
	mtctr r15
        lwz r15, CT_CR(r14)
        mtcr r15

	LDR r15, CT_HSRR0(r14)
	mtsrr0 r15
	LDR r15, CT_HSRR1(r14)
	mtsrr1 r15
	
        RESTORE_REGISTER(r4)
        RESTORE_REGISTER(r5)
        RESTORE_REGISTER(r6)
        RESTORE_REGISTER(r7)
        RESTORE_REGISTER(r8)
        RESTORE_REGISTER(r9)
        RESTORE_REGISTER(r10)
        RESTORE_REGISTER(r11)
        RESTORE_REGISTER(r12)
        RESTORE_REGISTER(r2)
        RESTORE_REGISTER(r1)
        RESTORE_REGISTER(r16)
        RESTORE_REGISTER(r15)
        RESTORE_REGISTER(r14)
        mfspr r13, SPRN_HSPRG1
	RFID
	
not_vec0:
	andi. r12,r3,0x7000
	cmpi cr0,r12,0x6000
	bne bad_hcall
	andi. r0,r3,0x0fff
	RLICR r0,r0,HCALL_VEC_IDX_SHIFT,HCALL_VEC_MASK_NUM
	LDR r12,TCA_CACHED_HCA(r13)
        LDR r12,HCA_HCALL_VEC6000_LEN(r12)
	CMPL r0,r12
	LDR r12,TCA_CACHED_HCA(r13)
        LDR r12,HCA_HCALL_VEC6000(r12)
	blt call_vec

bad_hcall:
	li r3,H_Function
	b return_no_preempt
	.long 0

#ifdef USE_GDB_STUB
ex_gdb_save:
	/*
	 * at this point:
	 *  - r1 is in hsprg1
	 *  - r2, r3 and the ctr are saved
	 *  - r3 has the exception vector we hit
	 *  - r1 has the address of cpu_state
	 */
	/* Save non-gpr volatiles */
	mflr	r2
	STR	r2, GDB_LR(r1)

	mfxer	r2	
	stw	r2, GDB_XER(r1)	/* 32b reg */

	mfcr	r2
	stw	r2, GDB_CR(r1)	/* 32b reg */

	mfsrr0	r2
	STR	r2, GDB_PC(r1)	

	mfsrr1	r2
	STR	r2, GDB_MSR(r1)	

	mfdar	r2
	STR	r2, GDB_DAR(r1)	

	mfdsisr	r2
	STR	r2, GDB_DSISR(r1)	

	mfspr	r2, SPRN_HSRR0	
	STR	r2, GDB_HSRR0(r1)	

	mfspr	r2, SPRN_HSRR1
	STR	r2, GDB_HSRR1(r1)	

	mfspr	r2, SPRN_HDEC
	STR	r2, GDB_HDEC(r1)	
	
	SAVE_GPR(r1,r0)
	SAVE_4GPR(r1,r4)
	SAVE_8GPR(r1, r8)
	SAVE_16GPR(r1,r16)

	mfspr	r4, SPRN_HSPRG1	  /* get original r1 */
	STR	r4, REG_WIDTH(r1) /* save to cpu_state.gpr[1] */

	mr	r4, r3		/* the vector is the second arg */
	
	mr	r3, r1
	subi	r1, r1, MIN_FRAME_SZ
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
	
	ld	r4, (3 * REG_WIDTH) (r3) /* r3-> sprg1 */
	mtsprg1	r4
	LOAD_2GPR(r3, r0)
	LOAD_4GPR(r3, r4)
	LOAD_8GPR(r3, r8)
	LOAD_16GPR(r3, r16)

	addi	r2, r2, 32
	mtspr	SPRN_HDEC, r2
	LOAD_GPR(r3, r2)
	mfsprg1	r3
	rfid

	.long 0


GLBL_LABEL(trap_instruction)
	tw	31,0,0
#endif
