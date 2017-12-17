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

#ifndef _HYPE_ASM_H
#define _HYPE_ASM_H

#ifndef CPU_4xx
#error this header is only for 4xx class processors
#endif

#ifdef __ASSEMBLY__
#include_next <hype_asm.h>

/*
 * Compute the OS virtual address of the vector to transfer to,
 * where rpcop is a pointer to the per_cpu_os structure and rvec
 * is the place to put the computed vector.
 */
#define COMPUTE_OS_VECTOR(rpcop, rvec, vector, ivornum) \
	LDR rvec,ivornum(rpcop)

/*
 * Save registers in preparation for reflecting an critical interrupt vector
 * to the current OS.  Leaves r3 pointing at the per_cpu_os structure.
 */
#define REFLECTC_TO_OS_SAVE(vector, ivornum) \
	SAVE_GPRS_3_4_5(); \
	SAVE_CR(r3, r4); \
	SAVE_CSRRS_TO_OSCOMM(r3,r4,r5); \
	COMPUTE_OS_VECTOR(r3, r5, vector, ivornum); \
	mtsrr0 r5; \
	LDR r5,PCO_EXCEPTION_MSR(r3); \
	mtsrr1 r5

/*
 * Reflect a critical interrupt vector to the current OS.
 */

#define REFLECTC_TO_OS(vector, ivornum) \
	REFLECTC_TO_OS_SAVE(vector, ivornum); \
	CHECK_PREEMPT(vector, r3, r4, preempt_3_4_5); \
	RESTORE_CR(r3, r4); \
	RESTORE_GPRS_3_4_5(); \
	RFI

/* 
 * Save the OS-specific special registers into the per_cpu_os structure
 * specified by rpcop, using rscratch as a scratch register.  The rpcop
 * and rscratch registers must be distinct.  The rpcop register is
 * preserved.  Note that (as always) rpcop may -not- be r0.
 */
#define SAVE_OS_SPECIAL(rpcop, rscratch1, rscratch2) \
	SAVE_OS_SPECIAL_COMMON(rpcop, rscratch1) \
	mfspr rscratch1,SPRN_DEAR; \
	mftbu rscratch1; \
	mftbl rscratch2; \
	STR rscratch2,REG_TB+4(rpcop); \
	mftbu rscratch2; \
	CMPL rscratch1,rscratch2; \
	bne .-20; \
	STR rscratch2,REG_TB(rpcop); \
	mfspr rscratch1,SPRN_ESR; \
	STR rscratch1,REG_ESR(rpcop); \
	mfspr rscratch1,SPRN_MMUCR; \
	STR rscratch1,REG_MMUCR(rpcop); \
	mfspr rscratch1,SPRN_PID; \
	STR rscratch1,REG_PID(rpcop); \
	mfsprg4 rscratch1; \
	STR rscratch1,REG_SPRG+4*REG_WIDTH(rpcop); \
	mfsprg5 rscratch1; \
	STR rscratch1,REG_SPRG+5*REG_WIDTH(rpcop); \
	mfsprg6 rscratch1; \
	STR rscratch1,REG_SPRG+6*REG_WIDTH(rpcop); \
	mfsprg7 rscratch1; \
	STR rscratch1,REG_SPRG+7*REG_WIDTH(rpcop); \
	mfspr rscratch1,SPRN_USPRG0; \
	STR rscratch1,REG_USPRG+0*REG_WIDTH(rpcop);

/* 
 * Restore the OS-specific special registers from the per_cpu_os structure
 * specified by rpcop, using rscratch as a scratch register.  The rpcop
 * and rscratch registers must be distinct.  The rpcop register is
 * preserved.  Note that (as always) rpcop may -not- be r0.
 */
#define RESTORE_OS_SPECIAL(rpcop, rscratch) \
	RESTORE_OS_SPECIAL_COMMON(rpcop, rscratch) \
	LDR rscratch,REG_DEAR(rpcop); \
	mtspr SPRN_DEAR,rscratch; \
	LDR rscratch,REG_ESR(rpcop); \
	mtspr SPRN_ESR,rscratch; \
	LDR rscratch,REG_MMUCR(rpcop); \
	mtspr SPRN_MMUCR,rscratch; \
	LDR rscratch,REG_PID(rpcop); \
	mtspr SPRN_PID,rscratch; \
	LDR rscratch,REG_SPRG+4*REG_WIDTH(rpcop); \
	mtsprg4 rscratch; \
	LDR rscratch,REG_SPRG+5*REG_WIDTH(rpcop); \
	mtsprg5 rscratch; \
	LDR rscratch,REG_SPRG+6*REG_WIDTH(rpcop); \
	mtsprg6 rscratch; \
	LDR rscratch,REG_SPRG+7*REG_WIDTH(rpcop); \
	mtsprg7 rscratch; \
	LDR rscratch,REG_USPRG+0*REG_WIDTH(rpcop); \
	mtspr SPRN_USPRG0,rscratch;

#endif /* __ASSEMBLY__ */
#endif /* _HYPE_ASM_H */
