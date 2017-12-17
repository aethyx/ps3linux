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
#include <asm_defs.h>

#ifdef USE_GDB_STUB
	
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

#endif /* USE_GDB_STUB */
	
/*
 * Test code for OS boot and exception vectors.
 */
	.globl _text_start
	.globl _vec_start
	.globl _vec_end
	.globl _partition_info
_text_start:
	/*
	 * This is here so controller can start at zero.  Later we
	 * will nop this instruction so that if a test OS branches to 0x0
	 * somehow, it will evetually assert
	 */
	. = 0x0
	b __start
	b .assert_at_0

	/* at offset 10 is a pointer to the partition info */
	. = 0x10
_partition_info:
	.llong HYPE_PARTITION_INFO_MAGIC_NUMBER
        .llong pinfo


_vec_start:
	.llong 0

	## This space is used for various scribblings expecially for
	## processors with no HV state, It should be described better.

#define EXR_SRR0	( 0 * REG_WIDTH)
#define EXR_SRR1	( 1 * REG_WIDTH)
#define EXR_R21		( 2 * REG_WIDTH)
#define EXR_R22		( 3 * REG_WIDTH)
#define EXR_R23		( 4 * REG_WIDTH)
#define EXR_DAR		( 5 * REG_WIDTH)
#define EXR_DSISR	( 6 * REG_WIDTH)
#define EXR_CR   	( 7 * REG_WIDTH)
#define EXR_HANDLER   	( 8 * REG_WIDTH)
#define EXR_VEC   	( 9 * REG_WIDTH)
#define EXR_LR		(10 * REG_WIDTH)
#define EXR_STACK_SAVE	(11 * REG_WIDTH)

#define EX_PROLOG(vec, xh, vrel)					\
	mtsprg	2,r22;				/* save r22 */		\
	LOADADDR(r22, xh_table);					\
	LDR	r22,(xh * REG_WIDTH)(r22);	/* lookup xh */		\
	mtsprg	1,r21;				/* save r21 */		\
	mfcr	r21;				/* save CR in r21 */	\
	CMPI	r22,0;	/* r22 is vaddr of exception handler */		\
	beq	1f;	/* C handler is NULL, don't call */		\
									\
	mtcr	r21;				/* put back cr */	\
	/* update exception stack */					\
	mfsprg	r21,3;							\
	addi	r21,r21,-EXR_STACK_SAVE;				\
	mtsprg	3,r21;							\
	/* save some regs and info in exception stack */		\
	STR	r22,EXR_HANDLER(r21);		/* save vaddr of xh */	\
	li	r22,vec;						\
	STR	r22,EXR_VEC(r21);		/* save vector */	\
	mfsprg	r22,1;				/* save orig r21 */	\
	STR	r22,EXR_R21(r21);					\
	mfsprg  r22,2;				/* save orig r22 */	\
	STR	r22,EXR_R22(r21);					\
	STR	r23,EXR_R23(r21);		/* save r23 */		\
	b	vrel;							\
1:	

### This is the default nothing to do return from interrupt now frag.
#define EX_FAST_RFI 							\
	mtcr	r21;				/* put back cr */	\
	mfsprg	r22,2;							\
	mfsprg	r21,1;							\
	RFI;								\
	
	
### Vectors start here
	
	. = 0x100		# System Reset
ex_sysreset:
	/*
	 * We allow the patition to be started here, we will NOP it
	 * in __start
	 */
	b __start
	EX_PROLOG(0x100, XH_SYSRESET, ex_xlate)
	EX_FAST_RFI
	
	. = 0x200		# Machine check
ex_machcheck:
	EX_PROLOG(0x200, XH_MACHCHECK, ex_xlate)
	EX_FAST_RFI

	. = 0x300		# DSI (data page fault)
ex_dsi:
	EX_PROLOG(0x300, XH_DSI, ex_no_xlate)
	EX_FAST_RFI

	. = 0x380		# Data SLB fault
ex_data_slb:
	EX_PROLOG(0x380, XH_DATA_SLB, ex_xlate)
	EX_FAST_RFI

	. = 0x400		# ISI (instruction page fault)
ex_isi:
	EX_PROLOG(0x400, XH_ISI, ex_no_xlate)
	EX_FAST_RFI

	. = 0x480		# Instruction SLB fault
ex_inst_slb:
	EX_PROLOG(0x480, XH_INST_SLB, ex_xlate)
	EX_FAST_RFI

	. = 0x500		# External interrupt
ex_interrupt:
	EX_PROLOG(0x500, XH_INTERRUPT, ex_xlate);
	EX_FAST_RFI

	. = 0x600		# Alignment
ex_alignment:
	EX_PROLOG(0x600, XH_ALIGNMENT, ex_xlate)
	EX_FAST_RFI

	. = 0x700		# Program exception (FP, illegal
ex_program:			# instruction, privileged inst...)
#ifndef USE_GDB_STUB
	EX_PROLOG(0x700, XH_PROGRAM, ex_xlate)
	EX_FAST_RFI
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
	mtspr	SPRN_SPRG1, r1
	LOADADDR(r1, gdb_currdbg)
	LDR	r1, 0(r1)
	
	subi	r1, r1, GDB_CPU_STATE_SIZE
	SAVE_2GPR(r1, r2)

	/* Save non-gpr volatiles */
	mflr	r2
	STR	r2, GDB_LR(r1)

	mfctr	r2
	STR	r2, GDB_CTR(r1)	

	mfxer	r2	
	stw	r2, GDB_XER(r1)	

	mfcr	r2
	stw	r2, GDB_CR(r1)	

	mfsrr0	r2
	STR	r2, GDB_PC(r1)	

	mfsrr1	r2
	STR	r2, GDB_MSR(r1)	

	mfdar	r2
	STR	r2, GDB_DAR(r1)	

	mfdsisr	r2
	STR	r2, GDB_DSISR(r1)
	
	/* save exception vector num in r2 */
	LOADADDR(r2, ex_gdb_save)
	mtctr	r2
	li	r2, 0x700
	bctr

#endif /* USE_GDB_STUB */
	
	. = 0x800		# FP unavailable
ex_float:
	EX_PROLOG(0x800, XH_FLOAT, ex_xlate)
	EX_FAST_RFI


	. = 0x900		# Decrementer interrupt
ex_dec:
	EX_PROLOG(0x900, XH_DEC, ex_xlate)
	b _xh_dec_fast
		
	. = 0x980		# HDEC
ex_hdec:
	EX_PROLOG(0x300, XH_DSI, ex_xlate)
	EX_FAST_RFI

	. = 0xc00		# Syscall interrupt
ex_syscall:
	EX_PROLOG(0xc00, XH_SYSCALL, ex_xlate)
	EX_FAST_RFI

	. = 0xd00		# Trace exception
ex_trace:
#ifndef USE_GDB_STUB
	EX_PROLOG(0xd00, XH_TRACE, ex_xlate)
	EX_FAST_RFI
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
	mtspr	SPRN_SPRG1, r1
	LOADADDR(r1, gdb_currdbg)
	LDR	r1, 0(r1)
	
	subi	r1, r1, GDB_CPU_STATE_SIZE
	SAVE_2GPR(r1, r2)

	/* Save non-gpr volatiles */
	mflr	r2
	STR	r2, GDB_LR(r1)

	mfctr	r2
	STR	r2, GDB_CTR(r1)	

	mfxer	r2	
	stw	r2, GDB_XER(r1)	

	mfcr	r2
	stw	r2, GDB_CR(r1)	

	mfsrr0	r2
	STR	r2, GDB_PC(r1)	

	mfsrr1	r2
	STR	r2, GDB_MSR(r1)
	
	mfdar	r2
	STR	r2, GDB_DAR(r1)	

	mfdsisr	r2
	STR	r2, GDB_DSISR(r1)	

	/* save exception vector num in r2 */
	LOADADDR(r2, ex_gdb_save)
	mtctr	r2
	li	r2, 0xd00
	bctr

#endif /* USE_GDB_STUB */
	. = 0xe00		# Floating-point assist
ex_fp:
	EX_PROLOG(0xe00, XH_FP, ex_xlate)
	EX_FAST_RFI

	. = 0x1000
ex_sony:
	EX_PROLOG(0x1000, XH_SONY, ex_xlate)
	EX_FAST_RFI

_vec_end:

	. = 0x2000
ex_generic:
	EX_PROLOG(0x2000, XH_GENERIC, ex_xlate)
	EX_FAST_RFI


_xh_dec_fast:
	mtcr	r21;		# put back cr
	## Simply resets the decrementer to shut it up
	LOADADDR(r21,decr_cnt)
	LDR	r22,0(r21)
	addi	r22,r22,1
	STR	r22,0(r21)
	lis	r22,5
	mtspr	SPRN_DEC,r22

#ifndef HAS_RMOR
	LOADADDR(r21, _vec_start)
	LDR	r22,0xc0(r21)
	mtsrr0	r22
	LDR	r22,0xc0+REG_WIDTH(r21)
	mtsrr1	r22
#endif
	mfcr	r21		# just so we can call EX_FAST_RFI
	EX_FAST_RFI

ex_save:
	## r21 ex_stack base
	## r22, r23 scratch
	## r22 return with the rfi PC
	mfcr	r22		# save CR in ex_stack
	STR	r22,EXR_CR(r21)
#ifdef DEBUG_STACKS
	LOADADDR(r22, ex_stackguard0)
	CMPL	r21, r22
	bge	1f		# jump over so I can set a breakpoint
	bl	.ex_stack_assert # So link register can show me the source
1:		
#endif
	mfdar	r22
	STR	r22,EXR_DAR(r21)
	mfdsisr	r22
	STR	r22,EXR_DSISR(r21)
	## store srr0 and srr1
	mfsrr0	r22
	STR	r22,EXR_SRR0(r21)
	mfsrr1	r22
	STR	r22,EXR_SRR1(r21)
	## get address of the common portion of this handler
	LOADADDR(r22, ex_common)
	## r22 return with the rfi PC
	blr

ex_xlate:
	## r21 ex_stack base
	## r22, r23 scratch
	mflr	r23
	STR	r23,EXR_LR(21)	# Save lr now so we can use it here
	bl	ex_save
	## r22 comes back with out rfi point
	## set up msr for xlate handler
	mfmsr	r23
#ifdef TARGET_LP64 
	rotldi  r23,r23,4
	## FIXME:	OOPS! What if the parition is not running with xlate on
	ori     r23,r23,0x30b		# Set IR, DR, SF, ISF, HV
	rotldi  r23,r23,60		# for generic handlers
#else
#error "no 32bit msr tickling code" 
#endif 
	mtsrr0	r22
	mtsrr1	r23
	RFI
	
	
ex_no_xlate:
	## r21 ex_stack base
	## r22, r23 scratch
	mflr	r23
	STR	r23,EXR_LR(r21)
	bl	ex_save
	## r22 comes back with out rfi point
	## set up msr for reloc handler
	mfmsr	r23	
	mtsrr0	r22
	mtsrr1	r23
	RFI

/* we rfi'd here with:
 * ex_stack in r21
 * r22, r23 are scratch and sprg1,2 are available
 */

/*
 * The ABI calling conventions let a procedure store all the non-volatile
 * registers beyond the end of the stack without moving the stack
 * pointer.  Interrupts and traps must preserve this area.  STK_FLOOR
 * is the lowest offset from the stack pointer that must not be
 * trampled.  Its magnitude is (19*8 + 18*8) because there are 19
 * non-volatile GPR's (r13 is questionable) and 18 non-volatile FPR's.
 * Since we want to preserve 16 byte alignement we consider 19 each.
 */
#define	STK_FLOOR	(19 * 8 * 2)
#define FRAME_SZ (MIN_FRAME_SZ + STK_FLOOR)
	.if ((FRAME_SZ & 0xf) <> 0)
	.err	# must be 16 byte aligned
	.endif
#define REG_SAVE_LAST XHR_MAX
	.if ((REG_SAVE_LAST & 0x1) <> 0)
	.err	# must be even as to keep stack 16 byte aligned
	.endif

#define REG_STACK_SZ  (((REG_SAVE_LAST) * REG_WIDTH) + FRAME_SZ)
	.if ((REG_STACK_SZ & 0xf) <> 0)
	.err	# must be 16 byte aligned
	.endif

	/* pattern to mark the stacks with */
#define REG_STACK_MARK	0xbeefdeadbeefdead
	
ex_common:
	mr	r23,r1			# move orig r1 to r23
### WARNING:
### This only works becauls virtual base = phys base = 0;
	## save and move stack ptr
	STRU	r1,-REG_STACK_SZ(r1)

	## store gprs
	STR	r4,((XHR_GPRS_4 * REG_WIDTH) + FRAME_SZ)(r1)
	addi	r4,r1,FRAME_SZ
#ifdef DEBUG_STACKS
	LOADADDR(r22, teststackguard0)
	CMPL	r4, r22
	bgt	1f
### Assumes r2 is valid.
	bl	.ex_stack_assert # lr can point out where we came from
1:		
#endif
	STR	r0,(XHR_GPRS_0 * REG_WIDTH)(r4)
	# r23 actually holds original r1?
	STR	r23,(XHR_GPRS_1 * REG_WIDTH)(r4)
	STR	r2,(XHR_GPRS_2 * REG_WIDTH)(r4)
	STR	r3,(XHR_GPRS_3 * REG_WIDTH)(r4)
	## r4 done above and will be passed to xh
	STR	r5,(XHR_GPRS_5 * REG_WIDTH)(r4)
	STR	r6,(XHR_GPRS_6 * REG_WIDTH)(r4)
	STR	r7,(XHR_GPRS_7 * REG_WIDTH)(r4)
	STR	r8,(XHR_GPRS_8 * REG_WIDTH)(r4)
	STR	r9,(XHR_GPRS_9 * REG_WIDTH)(r4)
	STR	r10,(XHR_GPRS_10 * REG_WIDTH)(r4)
	STR	r11,(XHR_GPRS_11 * REG_WIDTH)(r4)
	STR	r12,(XHR_GPRS_12 * REG_WIDTH)(r4)
	STR	r13,(XHR_GPRS_13 * REG_WIDTH)(r4)

	## move EX_STACK ptr to r5
	mr	r5,r21
	## restore the scratch regs
	LDR	r21,EXR_R21(r5)
	LDR	r22,EXR_R22(r5)
	LDR	r23,EXR_R23(r5)
	
	## other regs
	LDR	r3,EXR_CR(r5)
	STR	r3,(XHR_CR * REG_WIDTH)(r4)
	LDR	r3,EXR_LR(r5)
	STR	r3,(XHR_LR * REG_WIDTH)(r4)
	mfctr	r3
	STR	r3,(XHR_CTR * REG_WIDTH)(r4)
	mfxer	r3
	STR	r3,(XHR_XER * REG_WIDTH)(r4)

	## These don't need to be restored but we save them so the
	## exception handler has access to the.
	LDR	r3,EXR_DAR(r5)
	STR	r3,(XHR_DAR * REG_WIDTH)(r4)
	LDR	r3,EXR_DSISR(r5)
	STR	r3,(XHR_DSISR * REG_WIDTH)(r4)
	LDR	r3,EXR_SRR0(r5)
	STR	r3,(XHR_SRR0 * REG_WIDTH)(r4)
	LDR	r3,EXR_SRR1(r5)
	STR	r3,(XHR_SRR1 * REG_WIDTH)(r4)

	## get the vector value into parameter 1
	LDR	r3, EXR_VEC(r5)
	## get the handler's address
	LDR	r6, EXR_HANDLER(r5)

	## pop exception stack
	## FIXME do you need the stack to begin with?
	##   If yes, then perhaps you could get rid of earlier so we don't.
	mfsprg	r5,3
	addi	r5,r5,EXR_STACK_SAVE
	mtsprg	3,r5

#ifdef DEBUG
	## Mark the stack so we can check it later
	LOADCONST(r5, REG_STACK_MARK)
	STR	r5,(XHR_MARK * REG_WIDTH)(r4)
#endif
		
### FIXME: consider building a valid "traceback" frame or setting
### 	backchain to 0

	## r3 already has the exception vector
	## r4 already has the S/R ptr
	## call xh in r6
	CALL_CFUNC(r6)

### FIXME:	C function will S/R non-volatiles so you don't have to. Will need to restore scratch registers tho.
	## return from xh
	addi	r4,r1,FRAME_SZ
#ifdef DEBUG
	LOADCONST(r3, REG_STACK_MARK)
	LDR	r2,(XHR_MARK * REG_WIDTH)(r4)
	CMPL	r3,r2
	bge	1f		# jump over so I can set a breakpoint
	bl	.ex_stack_assert # So link register can show me the source
1:	
#endif
	LDR	r0,(XHR_GPRS_0 * REG_WIDTH)(r4)
	## restore r1 (stack) later
	LDR	r2,(XHR_GPRS_2 * REG_WIDTH)(r4)
	## restore r3 (scratch) later
	## restore r4 (save ptr) later
	LDR	r5,(XHR_GPRS_5 * REG_WIDTH)(r4)
	LDR	r6,(XHR_GPRS_6 * REG_WIDTH)(r4)
	LDR	r7,(XHR_GPRS_7 * REG_WIDTH)(r4)
	LDR	r8,(XHR_GPRS_8 * REG_WIDTH)(r4)
	LDR	r9,(XHR_GPRS_9 * REG_WIDTH)(r4)
	LDR	r10,(XHR_GPRS_10 * REG_WIDTH)(r4)
	LDR	r11,(XHR_GPRS_11 * REG_WIDTH)(r4)
	LDR	r12,(XHR_GPRS_12 * REG_WIDTH)(r4)
	LDR	r13,(XHR_GPRS_13 * REG_WIDTH)(r4)

	LDR	r3,(XHR_CR * REG_WIDTH)(r4)
	mtcr	r3
	LDR	r3,(XHR_LR * REG_WIDTH)(r4)
	mtlr	r3
	LDR	r3,(XHR_CTR * REG_WIDTH)(r4)
	mtctr	r3
	LDR	r3,(XHR_XER * REG_WIDTH)(r4)
	mtxer	r3
	## This is not needed but I want to see it happen
	LDR	r3,(XHR_DAR * REG_WIDTH)(r4)
	mtdar	r3

### BIG!!FIXME: we could be in xlate mode and so rfi'ing is difficult so we
### 	need make sure that there is no possibility of a page fault.
	
	LDR	r3,(XHR_SRR0 * REG_WIDTH)(r4)
	mtsrr0	r3
	LDR	r3,(XHR_SRR1 * REG_WIDTH)(r4)
	mtsrr1	r3
	LDR	r3,(XHR_GPRS_3 * REG_WIDTH)(r4)
	LDR	r4,(XHR_GPRS_4 * REG_WIDTH)(r4)
	## It might be interesting to see if XHR_GPRS_1 and r1 match
	LDR	r1,0(r1)
	RFI

	/*
	 * This makes the linker and elf loaders happy
	 */
C_TEXT_ENTRY(_start)
	b __start

	/*
	 * Finaly our real entry point
	 */	
__start:
	/*
	 * write a nop instruction at all out initial entry points
	 * so if they are called the right thing happens.
	 */
	bl 1f
	nop
1:	mflr	r1		# lr now contain the address of the nop above
	lwz	r1, 0(r1)	# r1 now contains the nop insn
	stw	r1, 0(r0)	# nop out 0x0
	/*stw	r1, 0x100(r0)	# nop out sysreset */
        mfspr r12, SPRN_PIR;
        cmpi    cr0,r12,0
        beq     thread1
        LOADADDR(r1,teststack0)
        LOADADDR(r10,ex_stack0)
        b keepgoing
thread1:
        LOADADDR(r1,teststack1)
        LOADADDR(r10,ex_stack1)
keepgoing:
	
	## initialize exceptions stack
	mtsprg	3,r10
	
	li r2,0
	STR r2, 0(r1)

	## set up decrementer, enought to shut it up by default
	lis r10,5
	mtspr	SPRN_DEC,r10

#ifdef  CPU_4xx
	lis	r10,SHIFT_R(SPRN_TCR_DIE,16)
	mtspr	SPRN_TCR,r10
#endif

	LOADADDR(r10,crt_init)
	CALL_CFUNC(r10)
	b .			# should not get here...

/*
 * leap_to():	used to call a function with the MSR in the right mode.
 * will pass arguement regisers r3 - r6 and leap at the function at
 * ptr r7 and set the msr to r8
 */
C_TEXT_ENTRY(leap_to)
#ifdef TARGET_LP64
	ld	r8, 0(r8)
#endif
	mtsrr0	r8		# return to func
	mtsrr1	r9;		# set up new msr
	RFI



#ifdef USE_GDB_STUB
ex_gdb_save:
	/* r1 is in sprg1, r2, r3 ctr, cr, lr, xer srr0, srr1 are saved */
	/* r2 has the exception vector we hit */
	/* r1 has the address of cpu_state */
	SAVE_GPR(r1,r0)
	SAVE_4GPR(r1,r4)
	SAVE_8GPR(r1, r8)
	SAVE_16GPR(r1,r16)

	mfspr	r4, SPRN_SPRG1	  /* get original r1 */
	STR	r4, REG_WIDTH(r1) /* save to cpu_state.gpr[1] */
	
	mr	r3, r1
	subi	r1, r1, MIN_FRAME_SZ
	mr	r4, r2
	LOADADDR(r5,enter_gdb)
	CALL_CFUNC(r5)
	nop

	ld	r10, GDB_MSR(r3)
	ld	r11, GDB_PC(r3)
	lwz	r12, GDB_CR(r3)
	lwz	r13, GDB_XER(r3)
	ld	r14, GDB_CTR(r3)
	ld	r15, GDB_LR(r3)

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
	LOAD_GPR(r3, r2)
	LOAD_4GPR(r3, r4)
	LOAD_8GPR(r3, r8)
	LOAD_16GPR(r3, r16)
	mfsprg1	r3
	rfid

	.long 0


GLBL_LABEL(trap_instruction)
	tw	31,0,0
#endif

/*
 * Start of Data Segment
 */
#define TEST_STACK_SIZE 0x80000
DATA_ENTRY(3)

	.fill (EXR_STACK_SAVE * 2), 4, 0xdeadbeef
ex_stackguard0:
	.space (EXR_STACK_SAVE * 10)
ex_stack0:
	
	## Used as stack for this test os
	.fill  (EXR_STACK_SAVE * 2), 4, 0xdeadbeef
teststackguard0:
	.space TEST_STACK_SIZE - MIN_FRAME_SZ
teststack0:
	.space MIN_FRAME_SZ
	.fill (EXR_STACK_SAVE * 2), 4, 0xdeadbeef
ex_stackguard1:
	.space (EXR_STACK_SAVE * 10)
ex_stack1:
	
	## Used as stack for this test os
	.fill  (EXR_STACK_SAVE * 2), 4, 0xdeadbeef
teststackguard1:
	.space TEST_STACK_SIZE - MIN_FRAME_SZ
teststack1:
	.space MIN_FRAME_SZ
