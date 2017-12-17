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
#ifndef __VREGS_H
#define __VREGS_H

#define VREG_BASE	(ULL(0) - ULL(0x1000))

#define NUM_MAP_CLASSES	8
#define NUM_EXC_VECTORS	16
#define INVALID_CLASS	ULL(~0)

#define NUM_EXC_SAVES	4

#define EXC_V_DEC	0
#define EXC_V_SYSCALL	1
#define EXC_V_PGFLT	2
#define EXC_V_FP	4
#define EXC_V_EXT	5
#define EXC_V_DEBUG	15

#define NUM_KERNEL_VMC	8

#define V_DEBUG_FLAG		1
#define V_DEBUG_MEM_FAULT	2

#ifndef __ASSEMBLY__

/* These are the registers HV saves on behalf of a partition and presents
 * them in memory to a kernel running in problem-state.
 */
struct vexc_save_regs
{
	uval reg_gprs[14] __align_64; /* r0 .. r12 */
	uval32 reg_xer;
	uval32 reg_cr;
	uval reg_lr;
	uval reg_ctr;
	uval v_srr0;
	uval v_srr1;
	uval32 prev_vsave;
	uval32 exc_num;
};

struct vregs {
	/* vexc_save must be first */
	struct vexc_save_regs vexc_save[NUM_EXC_SAVES];
	uval active_vsave; /* which vsave has just been used by hv */
	uval v_msr;
	uval v_ex_msr; /* msr to be used in exceptions */
	uval v_sprg0;
	uval v_sprg1;
	uval v_sprg2;
	uval v_sprg3;
	sval32 v_dec;
	uval v_dar;
	uval v_tb_offset;
	uval v_pir;
	uval v_dsisr;
	uval v_active_cls[NUM_MAP_CLASSES];
	uval exception_vectors[NUM_EXC_VECTORS];

	/* If set to V_DEBUG_FLAG, invalid memory references will,
	 * result in V_DEBUG_MEM_FAULT being asserted, and next instruction
	 * executed.
	 */
	uval32 debug;

	uval busy_vsave;
};

static struct vregs* const hype_vregs = (typeof(hype_vregs))VREG_BASE;
#endif

#endif /* __VREGS_H */
