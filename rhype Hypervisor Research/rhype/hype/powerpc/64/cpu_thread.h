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

#ifndef _CPU_THREAD_H
#define _CPU_THREAD_H

#include <config.h>
#include <types.h>
#include <xirr.h>
#include <xir.h>
#include <hv_asm.h>
#include <atomic.h>

/*  THREADS_PER_CPU is now configurable, default is 2 */
#ifndef THREADS_PER_CPU
#  error THREADS_PER_CPU should be configured by autoconf
#endif

struct cpu;

struct slb_entry {
	uval slb_vsid;
	uval slb_esid;
};

/* thread state constants */
#define CT_STATE_OFFLINE     0x00
#define CT_STATE_WAITING     0x01
#define CT_STATE_EXECUTING   0x02

/* preempt reason constants */
#define CT_PREEMPT_RESCHEDULE		0x01	/* go through schedulers */

#define CT_PREEMPT_YIELD_SELF		0x02	/* yield - not-directed */
#define CT_PREEMPT_YIELD_DIRECTED	0x04	/* yield - directed */
#define CT_PREEMPT_CEDE			0x08	/* cede */

struct cpu_thread {
	/* Magic marker */
	uval32 eyecatch;
	lock_t lock;		/* Hard 32 bit align */
	struct xir_queue xirq;	/* Hard 32 bit align */
	struct xir_queue iic_xirq;	/* Hard 32 bit align */
	uval8 state;
	uval8 preempt;
	uval8 thr_num;		/* Thread index */
	uval8 cpu_num;		/* CPU index */
	uval16 local_aipc_idx;	/* id of local AIPC buf */

	/*
	 * ok this puts at 128 bit alignment for VMX registers
	 */
#ifdef HAS_VMX
	uval128 reg_vrs[32] __align_128;
	uval128 reg_vscr;
#endif
	/*
	 * next are all hard 64bit values
	 */

	/* this is timebase lower and upper as one member */
	uval64 reg_tb;

	/* Implementation specific features/registers */
	struct cpu_imp_regs imp_regs __align_64;

#ifdef HAS_FP
	/* Floating point registers since we do no computation there
	 * is no reason to make them "double" */
	uval64 reg_fprs[32] __align_64;
#endif
#ifdef HAS_HDECR
	/* These registers cannot possibly exist without the HDEC */
	uval64 reg_hsrr0 __align_64;
	uval64 reg_hsrr1 __align_64;
#endif
#ifdef HAS_ASR
	uval64 reg_asr __align_64;
#endif
#ifdef HAS_FP
	/* this is only a 32bit register but there is no way to store
	 * an fpu register by word so we store as double (stfd) */
	uval64 reg_fpscr __align_64;
#endif
	/*
	 * next are member that are of the natural register width,
	 * these are uval and pointers.  At the end of this you want
	 * an even number of entries.
	 */
	/* Arrays first since there keep us even */
	uval reg_gprs[32] __align_64;
	uval reg_sprg[4] __align_64;
#ifdef HAS_SWSLB
	struct slb_entry slb_entries[SWSLB_SR_NUM] __align_64;
#endif
	struct cpu *cpu;
	uval reg_lr;
	uval reg_ctr;
	uval reg_srr0;
	uval reg_srr1;
	uval reg_dar;

	/*
	 * next is 32bit members
	 */
	uval32 reg_dsisr;
	uval32 reg_dec;
	uval32 reg_cr;
	uval32 reg_xer;
#ifdef HAS_VMX
	uval32 reg_vrsave;
	uval32 _vr_pad;
#endif
};

extern void init_thr_imp(struct cpu_thread *thr);

#ifndef HAS_MEDIATED_EE
/* Mediated interrupts not implemented */
static inline uval
thread_set_MER(struct cpu_thread *thr, uval val)
{
	(void)thr;
	(void)val;
	return 0;
}
#endif

#endif /* _CPU_THREAD_H */
