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
 * Definitions for OS-description data structures.
 *
 */

#ifndef _HYP_GENERIC_OS_H
#define _HYP_GENERIC_OS_H

#ifndef __ASSEMBLY__

#include <config.h>
#include <types.h>
#include <dlist.h>
#include <cpu.h>
#include <atomic.h>
#include <hypervisor.h>

/* An LPAR_DIE event results in the lpar_event struct being detached from the
   OS.  The owner of the lpar_event is expected to clean it up at that point */
enum {
	LPAR_DIE = 0x1,
};

struct lpar_event {
	struct dlist le_list;
	uval le_mask;		/* mask of events that callback is invoked for */
	/* if return value is 1 -> keep le registered */
	uval (*le_func) (struct os * os, struct lpar_event * le, uval event);
};

extern sval register_event(struct os *os, struct lpar_event *le);
extern sval unregister_event(struct os *os, struct lpar_event *le);

extern void run_event(struct os *os, uval event);

extern struct cpu_thread *get_cur_thread(void);

struct os_events {
	lock_t oe_lock;
	struct dlist oe_list;
};

/* Now include arch_os.h to get the definition of struct os.
 * It would be nice to structure this in some other way, but only if
 * each arch gets to define struct os the way it sees fit will each
 * arach be able to do thing such as field alignment as is appropriate
 * for that arch.
 */
#include <arch_os.h>

extern struct os *ctrl_os;
static inline struct cpu *
get_os_cpu(struct os *lpar, uval idx)
{
	if (idx < ARRAY_SIZE(lpar->cpu)) {
		return lpar->cpu[idx];
	}
	return NULL;
}

static inline struct cpu_thread *
get_os_thread(struct os *os, uval cpu, uval thr)
{
	struct cpu *c = get_os_cpu(os, cpu);

	if (c != NULL) {
		if (thr < ARRAY_SIZE(c->thread)) {
			return &c->thread[thr];
		}
	}
	return NULL;
}

enum {
	THREAD_MATCH = 0,
	CPU_MATCH = 1,
	NO_MATCH = 2
};

static inline uval
is_colocated(struct cpu_thread *a, struct cpu_thread *b)
{
	if (a->thr_num == b->thr_num && a->cpu_num == b->cpu_num) {
		return THREAD_MATCH;
	}
	if (a->cpu_num == b->cpu_num) {
		return CPU_MATCH;
	}
	return NO_MATCH;
}

/* Given an arbitrary thread (source), identify a thread from "target"
   that we can run on the same cpu/thread */
static inline struct cpu_thread *
find_colocated_thread(struct cpu_thread *source, struct os *target)
{
	uval cpu = source->cpu_num;
	uval thr = source->thr_num;
	struct cpu_thread *thread;

	thread = get_os_thread(target, cpu, thr);
	if (thread == NULL) {
		/* Maybe this partition simply isn't multi-threaded */
		thread = get_os_thread(target, cpu, 0);
		if (thread == NULL) {
			/* Last chance is cpu 0, thread 0 */
			thread = get_os_thread(target, 0, 0);
		}
	}
	return thread;
}

#endif /* ! __ASSEMBLY__ */

#endif /* ! _HYP_OS_H */
