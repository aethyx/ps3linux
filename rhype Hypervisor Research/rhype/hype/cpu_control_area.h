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

#ifndef _CPU_CONTROL_AREA_H
#define _CPU_CONTROL_AREA_H

#include <types.h>
#include <atomic.h>
#include <cpu.h>

struct cpu_control_area {
	uval32 eye_catcher;
	uval32 thread_count;
	lock_t tlb_lock;
	uval32 tlb_index_randomizer;
	struct cpu *active_cpu;
};

extern void cpu_idle(void) __attribute__ ((noreturn));
extern uval cca_init(struct cpu_control_area *cca);
extern uval cca_table_init(uval n);
extern uval cca_table_enter(uval e, struct cpu_control_area *cca);
extern uval cca_hard_start(struct cpu *cpu, uval i, uval pc);
#endif /* _CPU_CONTROL_AREA_H */
