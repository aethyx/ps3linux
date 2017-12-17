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
#include <os.h>
#include <thread_control_area.h>
#include <util.h>
#include <gdbstub.h>

#define hvi_shift \
	((sizeof(hcall_vec) / 4 == 2) ? 1 : \
	 ((sizeof(hcall_vec) / 4 == 4) ? 2 : \
	  ((sizeof(hcall_vec) / 4 == 8) ? 3 : 4)))

int
main(void)
{
	DECLARE(TCA_CPU_THREAD,
		offsetof(struct thread_control_area,
			 active_thread) + 0 * REG_WIDTH);
	DECLARE(TCA_HYPE_STACK,
		offsetof(struct thread_control_area,
			 hypeStack) + 0 * REG_WIDTH);
	DECLARE(TCA_HYPE_TOC,
		offsetof(struct thread_control_area, hypeToc) + 0 * REG_WIDTH);
	DECLARE(TCA_CACHED_HCA,
		offsetof(struct thread_control_area,
			 cached_hca) + 0 * REG_WIDTH);
	DECLARE(TCA_CACHED_R13,
		offsetof(struct thread_control_area,
			 cachedR13) + 0 * REG_WIDTH);
	DECLARE(TCA_CACHED_R14,
		offsetof(struct thread_control_area,
			 cachedR14) + 0 * REG_WIDTH);
	DECLARE(TCA_LAST_HCALL,
		offsetof(struct thread_control_area,
			 lasthcall) + 0 * REG_WIDTH);
	DECLARE(TCA_LAST_FEW,
		offsetof(struct thread_control_area, lastfew) + 0 * REG_WIDTH);
	DECLARE(TCA_SIZE, sizeof (struct thread_control_area));

	DECLARE(CPU_OS, offsetof(struct cpu, os));

#ifdef HAS_SWSLB
	DECLARE(CT_SLB_ENTRIES, offsetof(struct cpu_thread, slb_entries));
#endif
	DECLARE(CT_GPRS,
		offsetof(struct cpu_thread, reg_gprs) + 0 * REG_WIDTH);
	DECLARE(CT_GPR0,
		offsetof(struct cpu_thread, reg_gprs) + 0 * REG_WIDTH);
	DECLARE(CT_GPR1,
		offsetof(struct cpu_thread, reg_gprs) + 1 * REG_WIDTH);
	DECLARE(CT_GPR2,
		offsetof(struct cpu_thread, reg_gprs) + 2 * REG_WIDTH);
	DECLARE(CT_GPR3,
		offsetof(struct cpu_thread, reg_gprs) + 3 * REG_WIDTH);
	DECLARE(CT_GPR4,
		offsetof(struct cpu_thread, reg_gprs) + 4 * REG_WIDTH);
	DECLARE(CT_GPR5,
		offsetof(struct cpu_thread, reg_gprs) + 5 * REG_WIDTH);
	DECLARE(CT_GPR6,
		offsetof(struct cpu_thread, reg_gprs) + 6 * REG_WIDTH);
	DECLARE(CT_GPR7,
		offsetof(struct cpu_thread, reg_gprs) + 7 * REG_WIDTH);
	DECLARE(CT_GPR8,
		offsetof(struct cpu_thread, reg_gprs) + 8 * REG_WIDTH);
	DECLARE(CT_GPR9,
		offsetof(struct cpu_thread, reg_gprs) + 9 * REG_WIDTH);
	DECLARE(CT_GPR10,
		offsetof(struct cpu_thread, reg_gprs) + 10 * REG_WIDTH);
	DECLARE(CT_GPR11,
		offsetof(struct cpu_thread, reg_gprs) + 11 * REG_WIDTH);
	DECLARE(CT_GPR12,
		offsetof(struct cpu_thread, reg_gprs) + 12 * REG_WIDTH);
	DECLARE(CT_GPR13,
		offsetof(struct cpu_thread, reg_gprs) + 13 * REG_WIDTH);
	DECLARE(CT_GPR14,
		offsetof(struct cpu_thread, reg_gprs) + 14 * REG_WIDTH);
	DECLARE(CT_GPR15,
		offsetof(struct cpu_thread, reg_gprs) + 15 * REG_WIDTH);
	DECLARE(CT_GPR16,
		offsetof(struct cpu_thread, reg_gprs) + 16 * REG_WIDTH);
	DECLARE(CT_GPR17,
		offsetof(struct cpu_thread, reg_gprs) + 17 * REG_WIDTH);
	DECLARE(CT_GPR18,
		offsetof(struct cpu_thread, reg_gprs) + 18 * REG_WIDTH);
	DECLARE(CT_GPR19,
		offsetof(struct cpu_thread, reg_gprs) + 19 * REG_WIDTH);
	DECLARE(CT_GPR20,
		offsetof(struct cpu_thread, reg_gprs) + 20 * REG_WIDTH);
	DECLARE(CT_GPR21,
		offsetof(struct cpu_thread, reg_gprs) + 21 * REG_WIDTH);
	DECLARE(CT_GPR22,
		offsetof(struct cpu_thread, reg_gprs) + 22 * REG_WIDTH);
	DECLARE(CT_GPR23,
		offsetof(struct cpu_thread, reg_gprs) + 23 * REG_WIDTH);
	DECLARE(CT_GPR24,
		offsetof(struct cpu_thread, reg_gprs) + 24 * REG_WIDTH);
	DECLARE(CT_GPR25,
		offsetof(struct cpu_thread, reg_gprs) + 25 * REG_WIDTH);
	DECLARE(CT_GPR26,
		offsetof(struct cpu_thread, reg_gprs) + 26 * REG_WIDTH);
	DECLARE(CT_GPR27,
		offsetof(struct cpu_thread, reg_gprs) + 27 * REG_WIDTH);
	DECLARE(CT_GPR28,
		offsetof(struct cpu_thread, reg_gprs) + 28 * REG_WIDTH);
	DECLARE(CT_GPR29,
		offsetof(struct cpu_thread, reg_gprs) + 29 * REG_WIDTH);
	DECLARE(CT_GPR30,
		offsetof(struct cpu_thread, reg_gprs) + 30 * REG_WIDTH);
	DECLARE(CT_GPR31,
		offsetof(struct cpu_thread, reg_gprs) + 31 * REG_WIDTH);

#ifdef HAS_FP
	DECLARE(CT_FPRS, offsetof(struct cpu_thread, reg_fprs));
	DECLARE(CT_FPR0,
		offsetof(struct cpu_thread, reg_fprs) + 0 * REG_WIDTH);
	DECLARE(CT_FPR1,
		offsetof(struct cpu_thread, reg_fprs) + 1 * REG_WIDTH);
	DECLARE(CT_FPR2,
		offsetof(struct cpu_thread, reg_fprs) + 2 * REG_WIDTH);
	DECLARE(CT_FPR3,
		offsetof(struct cpu_thread, reg_fprs) + 3 * REG_WIDTH);
	DECLARE(CT_FPR4,
		offsetof(struct cpu_thread, reg_fprs) + 4 * REG_WIDTH);
	DECLARE(CT_FPR5,
		offsetof(struct cpu_thread, reg_fprs) + 5 * REG_WIDTH);
	DECLARE(CT_FPR6,
		offsetof(struct cpu_thread, reg_fprs) + 6 * REG_WIDTH);
	DECLARE(CT_FPR7,
		offsetof(struct cpu_thread, reg_fprs) + 7 * REG_WIDTH);
	DECLARE(CT_FPR8,
		offsetof(struct cpu_thread, reg_fprs) + 8 * REG_WIDTH);
	DECLARE(CT_FPR9,
		offsetof(struct cpu_thread, reg_fprs) + 9 * REG_WIDTH);
	DECLARE(CT_FPR10,
		offsetof(struct cpu_thread, reg_fprs) + 10 * REG_WIDTH);
	DECLARE(CT_FPR11,
		offsetof(struct cpu_thread, reg_fprs) + 11 * REG_WIDTH);
	DECLARE(CT_FPR12,
		offsetof(struct cpu_thread, reg_fprs) + 12 * REG_WIDTH);
	DECLARE(CT_FPR13,
		offsetof(struct cpu_thread, reg_fprs) + 13 * REG_WIDTH);
	DECLARE(CT_FPR14,
		offsetof(struct cpu_thread, reg_fprs) + 14 * REG_WIDTH);
	DECLARE(CT_FPR15,
		offsetof(struct cpu_thread, reg_fprs) + 15 * REG_WIDTH);
	DECLARE(CT_FPR16,
		offsetof(struct cpu_thread, reg_fprs) + 16 * REG_WIDTH);
	DECLARE(CT_FPR17,
		offsetof(struct cpu_thread, reg_fprs) + 17 * REG_WIDTH);
	DECLARE(CT_FPR18,
		offsetof(struct cpu_thread, reg_fprs) + 18 * REG_WIDTH);
	DECLARE(CT_FPR19,
		offsetof(struct cpu_thread, reg_fprs) + 19 * REG_WIDTH);
	DECLARE(CT_FPR20,
		offsetof(struct cpu_thread, reg_fprs) + 20 * REG_WIDTH);
	DECLARE(CT_FPR21,
		offsetof(struct cpu_thread, reg_fprs) + 21 * REG_WIDTH);
	DECLARE(CT_FPR22,
		offsetof(struct cpu_thread, reg_fprs) + 22 * REG_WIDTH);
	DECLARE(CT_FPR23,
		offsetof(struct cpu_thread, reg_fprs) + 23 * REG_WIDTH);
	DECLARE(CT_FPR24,
		offsetof(struct cpu_thread, reg_fprs) + 24 * REG_WIDTH);
	DECLARE(CT_FPR25,
		offsetof(struct cpu_thread, reg_fprs) + 25 * REG_WIDTH);
	DECLARE(CT_FPR26,
		offsetof(struct cpu_thread, reg_fprs) + 26 * REG_WIDTH);
	DECLARE(CT_FPR27,
		offsetof(struct cpu_thread, reg_fprs) + 27 * REG_WIDTH);
	DECLARE(CT_FPR28,
		offsetof(struct cpu_thread, reg_fprs) + 28 * REG_WIDTH);
	DECLARE(CT_FPR29,
		offsetof(struct cpu_thread, reg_fprs) + 29 * REG_WIDTH);
	DECLARE(CT_FPR30,
		offsetof(struct cpu_thread, reg_fprs) + 30 * REG_WIDTH);
	DECLARE(CT_FPR31,
		offsetof(struct cpu_thread, reg_fprs) + 31 * REG_WIDTH);
	DECLARE(CT_FPSCR, offsetof(struct cpu_thread, reg_fpscr));
#endif /* HAS_FP */

#ifdef HAS_VMX
	DECLARE(CT_VSCR, offsetof(struct cpu_thread, reg_vscr));
	DECLARE(CT_VRSAVE, offsetof(struct cpu_thread, reg_vrsave));
	DECLARE(CT_VR0,
		offsetof(struct cpu_thread, reg_vrs) + 0 * VEC_REG_WIDTH);
	DECLARE(CT_VR1,
		offsetof(struct cpu_thread, reg_vrs) + 1 * VEC_REG_WIDTH);
	DECLARE(CT_VR2,
		offsetof(struct cpu_thread, reg_vrs) + 2 * VEC_REG_WIDTH);
	DECLARE(CT_VR3,
		offsetof(struct cpu_thread, reg_vrs) + 3 * VEC_REG_WIDTH);
	DECLARE(CT_VR4,
		offsetof(struct cpu_thread, reg_vrs) + 4 * VEC_REG_WIDTH);
	DECLARE(CT_VR5,
		offsetof(struct cpu_thread, reg_vrs) + 5 * VEC_REG_WIDTH);
	DECLARE(CT_VR6,
		offsetof(struct cpu_thread, reg_vrs) + 6 * VEC_REG_WIDTH);
	DECLARE(CT_VR7,
		offsetof(struct cpu_thread, reg_vrs) + 7 * VEC_REG_WIDTH);
	DECLARE(CT_VR8,
		offsetof(struct cpu_thread, reg_vrs) + 8 * VEC_REG_WIDTH);
	DECLARE(CT_VR9,
		offsetof(struct cpu_thread, reg_vrs) + 9 * VEC_REG_WIDTH);
	DECLARE(CT_VR10,
		offsetof(struct cpu_thread, reg_vrs) + 10 * VEC_REG_WIDTH);
	DECLARE(CT_VR11,
		offsetof(struct cpu_thread, reg_vrs) + 11 * VEC_REG_WIDTH);
	DECLARE(CT_VR12,
		offsetof(struct cpu_thread, reg_vrs) + 12 * VEC_REG_WIDTH);
	DECLARE(CT_VR13,
		offsetof(struct cpu_thread, reg_vrs) + 13 * VEC_REG_WIDTH);
	DECLARE(CT_VR14,
		offsetof(struct cpu_thread, reg_vrs) + 14 * VEC_REG_WIDTH);
	DECLARE(CT_VR15,
		offsetof(struct cpu_thread, reg_vrs) + 15 * VEC_REG_WIDTH);
	DECLARE(CT_VR16,
		offsetof(struct cpu_thread, reg_vrs) + 16 * VEC_REG_WIDTH);
	DECLARE(CT_VR17,
		offsetof(struct cpu_thread, reg_vrs) + 17 * VEC_REG_WIDTH);
	DECLARE(CT_VR18,
		offsetof(struct cpu_thread, reg_vrs) + 18 * VEC_REG_WIDTH);
	DECLARE(CT_VR19,
		offsetof(struct cpu_thread, reg_vrs) + 19 * VEC_REG_WIDTH);
	DECLARE(CT_VR20,
		offsetof(struct cpu_thread, reg_vrs) + 20 * VEC_REG_WIDTH);
	DECLARE(CT_VR21,
		offsetof(struct cpu_thread, reg_vrs) + 21 * VEC_REG_WIDTH);
	DECLARE(CT_VR22,
		offsetof(struct cpu_thread, reg_vrs) + 22 * VEC_REG_WIDTH);
	DECLARE(CT_VR23,
		offsetof(struct cpu_thread, reg_vrs) + 23 * VEC_REG_WIDTH);
	DECLARE(CT_VR24,
		offsetof(struct cpu_thread, reg_vrs) + 24 * VEC_REG_WIDTH);
	DECLARE(CT_VR25,
		offsetof(struct cpu_thread, reg_vrs) + 25 * VEC_REG_WIDTH);
	DECLARE(CT_VR26,
		offsetof(struct cpu_thread, reg_vrs) + 26 * VEC_REG_WIDTH);
	DECLARE(CT_VR27,
		offsetof(struct cpu_thread, reg_vrs) + 27 * VEC_REG_WIDTH);
	DECLARE(CT_VR28,
		offsetof(struct cpu_thread, reg_vrs) + 28 * VEC_REG_WIDTH);
	DECLARE(CT_VR29,
		offsetof(struct cpu_thread, reg_vrs) + 29 * VEC_REG_WIDTH);
	DECLARE(CT_VR30,
		offsetof(struct cpu_thread, reg_vrs) + 30 * VEC_REG_WIDTH);
	DECLARE(CT_VR31,
		offsetof(struct cpu_thread, reg_vrs) + 31 * VEC_REG_WIDTH);
#endif /* HAS_VMX */

	DECLARE(CT_PREEMPT, offsetof(struct cpu_thread, preempt));
	DECLARE(CT_DEC, offsetof(struct cpu_thread, reg_dec));
	DECLARE(CT_CR, offsetof(struct cpu_thread, reg_cr));
	DECLARE(CT_CTR, offsetof(struct cpu_thread, reg_ctr));
	DECLARE(CT_LR, offsetof(struct cpu_thread, reg_lr));

	DECLARE(CT_XER, offsetof(struct cpu_thread, reg_xer));
	DECLARE(CT_SRR0, offsetof(struct cpu_thread, reg_srr0));
	DECLARE(CT_SRR1, offsetof(struct cpu_thread, reg_srr1));
	DECLARE(CT_HSRR0, offsetof(struct cpu_thread, reg_hsrr0));
	DECLARE(CT_HSRR1, offsetof(struct cpu_thread, reg_hsrr1));
	DECLARE(CT_SPRG0,
		offsetof(struct cpu_thread, reg_sprg + 0 * REG_WIDTH));
	DECLARE(CT_SPRG1,
		offsetof(struct cpu_thread, reg_sprg + 1 * REG_WIDTH));
	DECLARE(CT_SPRG2,
		offsetof(struct cpu_thread, reg_sprg + 2 * REG_WIDTH));
	DECLARE(CT_SPRG3,
		offsetof(struct cpu_thread, reg_sprg + 3 * REG_WIDTH));
	DECLARE(CT_TB, offsetof(struct cpu_thread, reg_tb));

#ifdef HAS_ASR
	DECLARE(CT_ASR, offsetof(struct cpu_thread, reg_asr));
#endif
	DECLARE(CT_DAR, offsetof(struct cpu_thread, reg_dar));
	DECLARE(CT_DSISR, offsetof(struct cpu_thread, reg_dsisr));
	DECLARE(CT_CPU, offsetof(struct cpu_thread, cpu));

	DECLARE(PO_STATE, offsetof(struct os, po_state));
	DECLARE(PO_BOOT_MSR, offsetof(struct os, po_boot_msr));
	DECLARE(PO_EXCEPTION_MSR, offsetof(struct os, po_exception_msr));

	DECLARE(HCA_HCALL_VEC,
		offsetof(struct hype_control_area,
			 hcall_vector) + 0 * REG_WIDTH);
	DECLARE(HCA_HCALL_VEC_LEN,
		offsetof(struct hype_control_area,
			 hcall_vector_len) + 0 * REG_WIDTH);
	DECLARE(HCA_HCALL_VEC6000,
		offsetof(struct hype_control_area,
			 hcall_6000_vector) + 0 * REG_WIDTH);
	DECLARE(HCA_HCALL_VEC6000_LEN,
		offsetof(struct hype_control_area,
			 hcall_6000_vector_len) + 0 * REG_WIDTH);

#ifdef USE_GDB_STUB
	DECLARE(GDB_MSR, offsetof(struct cpu_state, msr));
	DECLARE(GDB_PC, offsetof(struct cpu_state, pc));
	DECLARE(GDB_CR, offsetof(struct cpu_state, cr));
	DECLARE(GDB_XER, offsetof(struct cpu_state, xer));
	DECLARE(GDB_CTR, offsetof(struct cpu_state, ctr));
	DECLARE(GDB_LR, offsetof(struct cpu_state, lr));
	DECLARE(GDB_DAR, offsetof(struct cpu_state, dar));
	DECLARE(GDB_DSISR, offsetof(struct cpu_state, dsisr));
	DECLARE(GDB_GPR0, offsetof(struct cpu_state, gpr[0]));
	DECLARE(GDB_HSRR0, offsetof(struct cpu_state, hsrr0));
	DECLARE(GDB_HSRR1, offsetof(struct cpu_state, hsrr1));
	DECLARE(GDB_HDEC, offsetof(struct cpu_state, hdec));
	DECLARE(GDB_CPU_STATE_SIZE, sizeof (struct cpu_state));
#endif
	return 0;
}
