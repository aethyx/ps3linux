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
 *
 * HV specific CPU definitions
 *
 */

#ifndef __HYPE_X86_CPU_H
#define __HYPE_X86_CPU_H

#ifndef __ASSEMBLY__

#include <config.h>
#include <types.h>
#include <regs.h>
#include <cpu_thread.h>
#include <atomic.h>

extern uval cpuid_features;
extern uval cpuid_processor;
extern uval max_phys_memory;
extern void arch_validate(struct cpu_thread *thread);

typedef uval64 slot_mask_t;

struct sched_pol {
	/* Identifies currently owned scheduling slots */
	slot_mask_t curr_set;

	/* Identifies slots that we own and cannot be yielded */
	slot_mask_t required;

	/* Identifies slots that we want yielded to us */
	slot_mask_t desired;

	uval next_lpid;		/* directed yield request */

	/*
	 * Is OS runnable from point of view of scheduler?
	 * This is a representation of the per_cpu_os's scheduling state,
	 * not the OS state as represented in pco_state
	 */
	uval runnable:1;
};

struct os;
struct cpu {
	struct sched_pol sched;	/* scheduling policy */
	uval eyecatch;
	struct os *os;
	char logical_cpu_num;
	char preempt;		/* need to preempt OS */
	struct cpu_thread thread[THREADS_PER_CPU];
};

extern void cpu_core_init(void);

#endif /* __ASSEMBLY__ */

#endif /* __HYPE_X86_CPU_H */
