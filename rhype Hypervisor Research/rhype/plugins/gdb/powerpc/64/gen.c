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
#include <util.h>
#include <gdbstub.h>
#include <vregs.h>

int
main(void)
{
	DECLARE(V_ACTIVE_AREA, offsetof(struct vregs, active_vsave));
	DECLARE(V_BUSY_AREA, offsetof(struct vregs, busy_vsave));
	DECLARE(V_EXC_SAVE, offsetof(struct vregs, vexc_save));
	DECLARE(VS_GPRS, offsetof(struct vexc_save_regs, reg_gprs[0]));
	DECLARE(VS_XER, offsetof(struct vexc_save_regs, reg_xer));
	DECLARE(VS_LR, offsetof(struct vexc_save_regs, reg_lr));
	DECLARE(VS_CR, offsetof(struct vexc_save_regs, reg_cr));
	DECLARE(VS_CTR, offsetof(struct vexc_save_regs, reg_ctr));
	DECLARE(VS_SRR0, offsetof(struct vexc_save_regs, v_srr0));
	DECLARE(VS_SRR1, offsetof(struct vexc_save_regs, v_srr1));
	DECLARE(VS_PREV_VSAVE, offsetof(struct vexc_save_regs, prev_vsave));

	DECLARE(VS_SIZE, sizeof(struct vexc_save_regs));

	DECLARE(V_MSR, offsetof(struct vregs, v_msr));
	DECLARE(V_EX_MSR, offsetof(struct vregs, v_ex_msr));
	DECLARE(V_SPRG0, offsetof(struct vregs, v_sprg0));
	DECLARE(V_SPRG1, offsetof(struct vregs, v_sprg1));
	DECLARE(V_SPRG2, offsetof(struct vregs, v_sprg2));
	DECLARE(V_SPRG3, offsetof(struct vregs, v_sprg3));
	DECLARE(V_EXC_ENTRY, offsetof(struct vregs, exception_vectors));
	DECLARE(V_DEC, offsetof(struct vregs, v_dec));
	DECLARE(V_DAR, offsetof(struct vregs, v_dar));
	DECLARE(V_TB_OFFSET, offsetof(struct vregs, v_tb_offset));
	DECLARE(V_DSISR, offsetof(struct vregs, v_dsisr));

	DECLARE(VS_EXC_NUM, offsetof(struct vexc_save_regs, exc_num));

	DECLARE(GDB_ZERO, 0);
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
