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

#ifndef _HYPE_H
#define _HYPE_H

#include <config.h>
#include <lib.h>
#include <os.h>
#include <pgalloc.h>

struct hype_scheduler {
	/* Bit-map to let us know which scheduling slots are in use */
	slot_mask_t used_slots;
	/* Bit-map to let us know which scheduling slots cannot be yielded */
	slot_mask_t locked_slots;

	/* Who owns which scheduling slot. */
	struct cpu *owners[NUM_SCHED_SLOTS];

	/* Which scheduling slot is active */
	uval curr_slot;
};

struct hype_per_cpu_s {
	struct hype_scheduler hpc_sched;
	uval hpc_nswtch;
	lock_t hpc_mutex;	/* 32 bit */
	char _pad[sizeof (uval) - 4];
};

extern void hypervisorInit(int on_hardware);

extern struct cpu *sched_next_os(struct cpu *);
extern sval32 read_hype_timer(void);
extern sval32 get_rtos_dec(struct cpu_thread *rtos_thread);

extern uval run_partition(struct cpu_thread *thread);
void run_cpu(struct cpu *);

extern void console(char *buf);
extern lock_t hype_mutex;

extern struct hype_per_cpu_s hype_per_cpu[MAX_CPU];
extern void os_init(void);
extern void os_free(struct os *pop);
extern struct os *os_create(void);
extern struct os *os_lookup(uval lpid);

/* arch stuff */
extern uval arch_os_init(struct os *newOS, uval pinfo_addr);
extern void arch_os_destroy(struct os *os);
extern uval core_os_init(struct os *newOS);
extern uval check_dec_preempt(struct cpu_thread *thread);
extern uval clear_exception_state(struct cpu_thread *thread);
extern uval force_intr_exception(struct cpu_thread *thread, uval ex_vector);
extern uval arch_preempt_os(struct cpu_thread *curThread,
			    struct cpu_thread *nextThread);

extern struct pg_alloc phys_pa;

extern void arch_init_thread(struct os *os, struct cpu_thread *thread,
			     uval pc, uval, uval, uval, uval, uval, uval);

#define hprintf_mt(fmt, args...) \
  hprintf("%x-%x:" fmt, get_proc_id(), get_thread_id(), ## args)

#endif /* ! _HYPE_H */
