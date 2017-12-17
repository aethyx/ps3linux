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
 *
 */

#include <config.h>
#include <hypervisor.h>
#include <hv_asm.h>
#include <lpar.h>
#include <os.h>
#include <asm_defs.h>


do_idle:
	/*
	 * Until there's a timer facility, just use the last hdecr
	 * setting of the last partition (which is >= SLOT_CYCLES).
	 * What do we do with decr?
	 * Note "myPerOS" hasn't changed -- still set to last run partition.
	 */
	mtdec	r4
	mfmsr	r5
	ori	r5, r5, MSR_EE
	mtmsr   r5
idle:	ATTN
	b	idle


C_TEXT_ENTRY(tlbiel)	
	.long 0x7C001A24; /* tlbiel 3*/
	blr;

C_TEXT_ENTRY(resume_thread_asm)	
        mr r14, r3
### Restore SRR0/1
        LDR r15, CT_SRR0(r14)
	mtsrr0 r15
        LDR r15, CT_SRR1(r14)
	mtsrr1 r15
	LDR r15,CT_LR(r14); 
	mtlr r15; 
	LDR r15,CT_CTR(r14); 
	mtctr r15; 
	lwz r15,CT_CR(r14); 
	mtcr r15; 
	lwz r15,CT_XER(r14); 
	mtxer r15
	LDR r4,CT_HSRR0(r14)
	mtspr  SPRN_HSRR0,r4
	LDR r4,CT_HSRR1(r14)
	mtspr  SPRN_HSRR1,r4
        RESTORE_REGISTER(r31)
        RESTORE_REGISTER(r30)
        RESTORE_REGISTER(r29)
        RESTORE_REGISTER(r28)
        RESTORE_REGISTER(r27)
        RESTORE_REGISTER(r26)
        RESTORE_REGISTER(r25)
        RESTORE_REGISTER(r24)
        RESTORE_REGISTER(r23)
        RESTORE_REGISTER(r22)
        RESTORE_REGISTER(r21)
        RESTORE_REGISTER(r20)
        RESTORE_REGISTER(r19)
        RESTORE_REGISTER(r18)
        RESTORE_REGISTER(r17)
        RESTORE_REGISTER(r16)
        RESTORE_REGISTER(r12)
        RESTORE_REGISTER(r11)
        RESTORE_REGISTER(r10)
        RESTORE_REGISTER(r9)
        RESTORE_REGISTER(r8)
        RESTORE_REGISTER(r7)
        RESTORE_REGISTER(r6)
        RESTORE_REGISTER(r5)
        RESTORE_REGISTER(r4)
        RESTORE_REGISTER(r3)
        RESTORE_REGISTER(r2)
        RESTORE_REGISTER(r1)
        RESTORE_REGISTER(r0)
        LDR r15, CT_GPR13(r14)
        mtspr SPRN_HSPRG1, r15
        RESTORE_REGISTER(r15)
        RESTORE_REGISTER(r14)
        mfspr r13, SPRN_HSPRG1
	HRFID
