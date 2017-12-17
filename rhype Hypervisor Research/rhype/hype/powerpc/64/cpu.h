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

#ifndef _POWERPC_64_CPU_H
#define _POWERPC_64_CPU_H

#include <config.h>
#include <types.h>
#include <atomic.h>
#include <cpu_thread.h>
#include_next <cpu.h>

typedef uval64 slot_mask_t;

struct sched_poll_s {
	/* Identifies currently owned scheduling slots. */
	slot_mask_t curr_set;

	/* Identifies slots that we own and cannot be yielded. */
	slot_mask_t required;

	/* Identifies slots that we want yielded to us. */
	slot_mask_t desired;

	uval next_lpid;		/* directed yield request */

	/* Is OS runnable from point of view of scheduler?
	 * This is a representation of the per_cpu_os's scheduling state,
	 * not the OS state as represented in pco_state
	 */
	uval runnable;		/* boolean if you need more room */
};

#define C_STATE_PREEMPTED   0x0
#define C_STATE_EXECUTING   0x2
#define C_STATE_PREEMPTING  0x3

struct os;
struct cpu {
	uval32 eyecatch;
	uval16 logical_cpu_num;
	uval8 state;
#ifdef HAS_TAGGED_TLB
	uval8 tlb_flush_pending;
#endif

	/* 64 bit aligned here */
	struct sched_poll_s sched;

	/* 128 bit aligned here */
	/* 128 bit members */
	struct cpu_thread thread[THREADS_PER_CPU];

	/* 128 bit aligned here */
#ifdef HAS_SPC_CPU_DATA
	struct spc_cpu_data spc_data[BP_SPUS];
	char _spc_pad[(__alignof__(struct spc_cpu_data) * BP_SPUS) % 16];
#endif
	/* 128 bit aligned here */
	/* 64 bit members */
	uval reg_sdr1;
	char _pad0[sizeof (uval) % 8];
	/* 128 bit aligned here */
	/* uval members */
	struct os *os;
	uval _pad1[3];
};

static inline uval64
srr1_set_sf(uval64 msr)
{
#ifdef HAS_MSR_ISF
	if ((msr & MSR_ISF) != 0) {
		msr |= MSR_SF;
	}
#else
	msr |= MSR_SF;
#endif
	return msr;
}

/* FIXME: Do this better.  We need:
 *  Architecture init
 *  Processor Init
 *  Board level implementation Init
 *  ...
 */
extern void cpu_core_init(uval ofd, uval cpuno, uval cpuseq);

#endif /* _POWERPC_64_CPU_H */
