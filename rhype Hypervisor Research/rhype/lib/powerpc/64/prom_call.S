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
#include <asm.h>

#define SAVE_GPR(reg, rn) STR  rn , (REG_WIDTH * (rn)) (reg)
#define SAVE_2GPR(reg, rn) SAVE_GPR(reg, rn) ;  SAVE_GPR(reg, rn+1)
#define SAVE_4GPR(reg, rn) SAVE_2GPR(reg, rn) ;  SAVE_2GPR(reg, rn + 2)
#define SAVE_8GPR(reg, rn) SAVE_4GPR(reg, rn) ;  SAVE_4GPR(reg, rn + 4)
#define SAVE_16GPR(reg, rn) SAVE_8GPR(reg, rn) ;  SAVE_8GPR(reg, rn + 8)

#define LOAD_GPR(reg, rn) LDR  rn , (REG_WIDTH * (rn)) (reg)
#define LOAD_2GPR(reg, rn) LOAD_GPR(reg, rn) ;  LOAD_GPR(reg, rn+1)
#define LOAD_4GPR(reg, rn) LOAD_2GPR(reg, rn) ;  LOAD_2GPR(reg, rn + 2)
#define LOAD_8GPR(reg, rn) LOAD_4GPR(reg, rn) ;  LOAD_4GPR(reg, rn + 4)
#define LOAD_16GPR(reg, rn) LOAD_8GPR(reg, rn) ;  LOAD_8GPR(reg, rn + 8)
#define GET_STORE(reg, n)  LOADADDR(reg, n)
/*
 * sval32 prom_call(void *arg, uval base, uval func,
 *			uval msr);
 * r3 is arg pointer	
 * r4 is RTAS base, should be 0 for OF
 * r5 is Prom vector
 * r6 is the MSR we should use
 */
C_TEXT_ENTRY(prom_call)
	GET_STORE(r7, gpr_store)

	SAVE_GPR(r7, r1)
	SAVE_GPR(r7, r2)
	SAVE_GPR(r7, r13)
	SAVE_16GPR(r7, r14)
	SAVE_2GPR(r7, r30)


	mflr    r18
	sradi	r19, r18, 32		/* store lr in r18, r19 */

	mfmsr   r20
	sradi	r21, r20, 32		/* r20,r21 contain caller's msr */

	/* save SRR0/1 */
	mfsrr0	r24
	sradi	r25, r24, 32
	mfsrr1	r26
	sradi	r27, r26, 32

	bl	go_to_real

	/* come back here after rfid in "native_of_call" */
	insrdi	r18, r19, 32, 0		/* restore lr */
	mtlr	r18			

	/* restore SRR0/1 */
	insrdi	r24, r25, 32, 0
	mtsrr0	r24
	insrdi	r26, r27, 32, 0
	mtsrr1	r26


	
	GET_STORE(r7, gpr_store)

	LOAD_GPR(r7, r1)
	LOAD_GPR(r7, r2)
	LOAD_GPR(r7, r13)
	LOAD_16GPR(r7, r14)
	LOAD_2GPR(r7, r30)

	
	blr

		
C_TEXT_END(prom_call)

native_call:	
	mtctr	r5
	bctrl				/* call of or rtas */

	insrdi	r22, r23, 32, 0		/* reconstruct return point iar */
	insrdi	r20, r21, 32, 0		/* reconstruct caller's msr */

	mtsrr0	r22
	mtsrr1	r20
	rfid


go_to_real:
	mflr	r22
	sradi	r23, r22, 32		/* save address we rfid back to*/

	LOADADDR(r8, native_call)

	mtsrr0	r8
	mtsrr1	r6
	rfid

			
	.data
	.p2align 3
gpr_store:
	.space 32 * REG_WIDTH

msr_store:
	.llong 0
	.llong 0x1000000000003000

branch_link:
	.llong 0	

	.previous

