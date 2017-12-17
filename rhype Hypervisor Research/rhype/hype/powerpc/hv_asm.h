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
#ifndef _POWERPC_HV_ASM_H
#define _POWERPC_HV_ASM_H

#include <asm.h>
#include <hv_regs.h>

/*
 * Restore the specified register(s), given a pointer to the per_cpu_os
 * structure in rpcoc.
 */
#define RESTORE_REGISTER(rn) LDR rn,CT_GPRS+(REG_WIDTH*(rn))(r14)

/*
 * Save the special registers given that register rpcop contains a
 * pointer to the per_cpu_os structure and that rscratch may be used
 * as a scratch register.
 */

#define SAVE_SPECIAL_VOLATILES(rscratch) \
	mflr rscratch; \
	STR rscratch,CT_LR(r14); \
	mfctr rscratch; \
	STR rscratch,CT_CTR(r14); \
	mfxer rscratch; \
	stw rscratch,CT_XER(r14)

/*
 * Restore the special registers given that register rpcop contains a
 * pointer to the per_cpu_os structure and that rscratch may be used
 * as a scratch register.
 */

#define RESTORE_SPECIAL_VOLATILES(rscratch) \
	LDR rscratch,CT_LR(r14); \
	mtlr rscratch; \
	LDR rscratch,CT_CTR(r14); \
	mtctr rscratch; \
	lwz rscratch,CT_XER(r14); \
	mtxer rscratch

#define HRFID	.long 0x4c000224

#ifndef __ASSEMBLY__
#include <types.h>
#include <msr.h>

static inline void
do_enter_hv_mode(void)
{
	/* *INDENT-OFF* */
	asm __volatile__("mr 3, %0    \n"
			 ".long 0x44000022\n"
			 :
			 :"r"(0xdedLL)
			 :"memory","r3");
	/* *INDENT-ON* */
}

static inline int
enter_hv_mode(void)
{
	uval msr;

	msr = mfmsr();
	if (!(msr & MSR_HV)) {
		do_enter_hv_mode();
		return 0;
	}
	return 1;
}

static void leave_hv_mode(void) __attribute__ ((no_instrument_function));

static inline void
leave_hv_mode(void)
{
	uval msr;

	msr = mfmsr();

	/* *INDENT-OFF* */
	asm __volatile__ (
		"mtsrr1 %0     \n" /* SPR 27 = SRR1 */
		"bl 1f            \n" 
		"1:               \n"
		"mflr 3           \n"
		"addi 3, 3, 24    \n"
		"mtsrr0 3       \n" /* SPR 26 = SRR0 */
		"rfid             \n"
		"nop              \n"
		"nop              \n"
		"nop              \n"
		"nop              \n"
		"nop              \n"
		:
		:"r"(msr & ~MSR_HV)
		:"r3");
	/* *INDENT-ON* */

}
#endif /* __ASSEMBLY__ */
#endif /* !_POWERPC_HV_ASM_H */
