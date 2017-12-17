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
 * exhaustive test for yield
 *
 */

#include <config.h>
#include <asm.h>
#include <lpar.h>

/* Save registers into an array. */

#define TSAVE_1_GPR(ptr, rn) \
	STR rn,0+(REG_WIDTH*(rn))(ptr)
#define TSAVE_2_GPR(ptr, rn) \
	TSAVE_1_GPR(ptr, rn); \
	TSAVE_1_GPR(ptr, rn+1)
#define TSAVE_4_GPR(ptr, rn) \
	TSAVE_2_GPR(ptr, rn); \
	TSAVE_2_GPR(ptr, rn+2)
#define TSAVE_8_GPR(ptr, rn) \
	TSAVE_4_GPR(ptr, rn); \
	TSAVE_4_GPR(ptr, rn+4)
#define TSAVE_16_GPR(ptr, rn) \
	TSAVE_8_GPR(ptr, rn); \
	TSAVE_8_GPR(ptr, rn+8)
#define TSAVE_ALL_GPR(ptr) \
	TSAVE_16_GPR(ptr, 0); \
	TSAVE_16_GPR(ptr, 16)

/* Compare registers to values previously saved in an array. */

#define TCHECK_1_GPR(ptr, rn, rscratch, label) \
	LDR	rscratch,0+(REG_WIDTH*(rn))(ptr); \
	CMPL	rn,rscratch; \
	beq	DOT+12; \
	li	rscratch,rn; \
	b	label
#define TCHECK_2_GPR(ptr, rn, rscratch, label) \
	TCHECK_1_GPR(ptr, rn, rscratch, label); \
	TCHECK_1_GPR(ptr, rn+1, rscratch, label)
#define TCHECK_4_GPR(ptr, rn, rscratch, label) \
	TCHECK_2_GPR(ptr, rn, rscratch, label); \
	TCHECK_2_GPR(ptr, rn+2, rscratch, label)
#define TCHECK_8_GPR(ptr, rn, rscratch, label) \
	TCHECK_4_GPR(ptr, rn, rscratch, label); \
	TCHECK_4_GPR(ptr, rn+4, rscratch, label)
#define TCHECK_16_GPR(ptr, rn, rscratch, label) \
	TCHECK_8_GPR(ptr, rn, rscratch, label); \
	TCHECK_8_GPR(ptr, rn+8, rscratch, label)
#define TCHECK_28_GPR(ptr, rn, rscratch, label) \
	TCHECK_16_GPR(ptr, rn, rscratch, label); \
	TCHECK_8_GPR(ptr, rn+8, rscratch, label); \
	TCHECK_4_GPR(ptr, rn+12, rscratch, label)

/*
 * Check the special register accessed by instruction "spec" against
 * the specified "offset" from "ptr", using "rs1" and "rs2"
 * as scratch registers.  Branch to "label" if mismatch, leaving the
 * "offset" in "rs2".  Clobbers cr0.
 */

#define TCHECK_SPECIAL(ptr, spec, offset, rs1, rs2, label) \
	spec	rs1; \
	TCHECK_REG(ptr, offset, rs1, rs2, label)

/*
 * Check the specified constant "const" against the specified "offset"
 * from "ptr", using "rs1" and "rs2" as scratch registers.  Branch to
 * "label" if mismatch, leaving the "offset" in "rs2".  Clobbers cr0.
 * Note that "const" is a signed 16-bit quantity.
 */

#define TCHECK_CONST(ptr, const, offset, rs1, rs2, label) \
	li	rs1,const; \
	TCHECK_REG(ptr, offset, rs1, rs2, label)

/*
 * Check "reg" against the specified "offset" from "ptr", using "rs2"
 * as a scratch register. Branch to "label" if mismatch, leaving the
 * "offset" in "rs2".  Clobbers cr0.
 */

#define TCHECK_REG(ptr, offset, reg, rs2, label) \
	LDR	rs2,offset(ptr); \
	CMPL	reg,rs2; \
	beq	DOT+12; \
	li	rs2,offset; \
	b	label

/*
 * debugging code to verify that context switch correctly saves and
 * restores registers.  First argument is lpid, second argument is
 * 32-element array into which to store GP registers.
 */

C_TEXT_ENTRY(hcall_yield_check_regs)
	addi	r5,r3,1
	addi	r6,r3,2
	addi	r7,r3,3
	addi	r8,r3,4
	addi	r9,r3,5
	addi	r10,r3,6
	addi	r11,r3,7
	addi	r12,r3,8
	## save all GPRS
	STR	r0,(0 * REG_WIDTH)(r4)   
	STR	r1,(1 * REG_WIDTH)(r4)   
	STR	r2,(2 * REG_WIDTH)(r4)  
	STR	r3,(3 * REG_WIDTH)(r4)  
	STR	r4,(4 * REG_WIDTH)(r4)  
	STR	r5,(5 * REG_WIDTH)(r4)  
	STR	r6,(6 * REG_WIDTH)(r4)  
	STR	r7,(7 * REG_WIDTH)(r4)  
	STR	r8,(8 * REG_WIDTH)(r4)  
	STR	r9,(9 * REG_WIDTH)(r4)  
	STR	r10,(10 * REG_WIDTH)(r4) 
	STR	r11,(11 * REG_WIDTH)(r4) 
	STR	r12,(12 * REG_WIDTH)(r4) 
	STR	r13,(13 * REG_WIDTH)(r4)
	STR	r14,(14 * REG_WIDTH)(r4)
	STR	r15,(15 * REG_WIDTH)(r4)
	STR	r16,(16 * REG_WIDTH)(r4)
	STR	r17,(17 * REG_WIDTH)(r4)
	STR	r18,(18 * REG_WIDTH)(r4)
	STR	r19,(19 * REG_WIDTH)(r4)
	STR	r20,(20 * REG_WIDTH)(r4)
	STR	r21,(21 * REG_WIDTH)(r4)
	STR	r22,(22 * REG_WIDTH)(r4)
	STR	r23,(23 * REG_WIDTH)(r4)
	STR	r24,(24 * REG_WIDTH)(r4)
	STR	r25,(25 * REG_WIDTH)(r4)
	STR	r26,(26 * REG_WIDTH)(r4)
	STR	r27,(27 * REG_WIDTH)(r4)
	STR	r28,(28 * REG_WIDTH)(r4)
	STR	r29,(29 * REG_WIDTH)(r4)
	STR	r30,(30 * REG_WIDTH)(r4)
	STR	r31,(31 * REG_WIDTH)(r4)

#define SR_LR	((1 * REG_WIDTH) + MIN_FRAME_SZ)
#define SR_CTR	((2 * REG_WIDTH) + MIN_FRAME_SZ)
#define SR_R4	((3 * REG_WIDTH) + MIN_FRAME_SZ)
#define SR_SAVE	((4 * REG_WIDTH) + MIN_FRAME_SZ)
stack_init:
	## make room on stack
	STRU	r1, -SR_SAVE(r1)
	## save some specials
	mflr	r0
	STR	r0,SR_LR(r1)
	mfctr	r0
	STR	r0,SR_CTR(r1)
	STR	r4,SR_R4(r1)
	li	r3,H_YIELD
	li	r4,H_SELF_LPID
	HSC

check_hcall:
	##  save cr and check return of hcall
	mfcr	r0
	CMPI	r3,0
	beq	check_special
	blr
### 
### NOTE:
### GPRS 0, 3-12, CR, XER and CTR are volatile in hcall()s.
### ABI says that LR is volatile as well.
### 
check_special:
	# r0 has been checked, and has cr, but cr is volatile
	mflr	r0
	LDR	r3,SR_LR(r1)
	CMPL	r0,r3
	li	r0,-2
	bne	yield_check_regs_fail
	
check_gprs:	
#define CHECK(reg)				\
	LDR	r0,(reg * REG_WIDTH)(r4);	\
	CMPL	r0,reg;				\
	li	r0,reg;				\
	bne	yield_check_regs_fail

	##  bring back r4
	LDR	r4,SR_R4(r1)
	## lets just get these right ok?
	CHECK(2)
	CHECK(13)
	CHECK(14)
	CHECK(15)
	CHECK(16)
	CHECK(17)
	CHECK(18)
	CHECK(19)
	CHECK(20)
	CHECK(21)
	CHECK(22)
	CHECK(23)
	CHECK(24)
	CHECK(25)
	CHECK(26)
	CHECK(27)
	CHECK(28)
	CHECK(29)
	CHECK(30)
	CHECK(31)

	/* put the stack back */
	LDR	r1,0(r1)
	li	r3,H_Success
	blr

yield_check_regs_fail:
	STR	r0,0(r4)		/* Record the offending register ID. */
	li	r3,H_Hardware
	blr
