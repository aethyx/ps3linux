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
#ifndef __HYPE_X86_OS_H__
#define __HYPE_X86_OS_H__

#ifndef _HYP_GENERIC_OS_H
#error	"arch_os.h must not be included directly. Include os.h instead."
#endif

#ifndef __ASSEMBLY__

#include <lpar.h>
#include <ipc.h>
#include <cpu.h>
#include <asm.h>
#include <io_chan.h>
#include <logical.h>
#include <vtty.h>
#include <pgframe.h>
#include <partition.h>
#include <aipc.h>
#include <vm.h>

/*
 * Hypervisor-call vector. Ins is the number of 32-bit words
 * that are copied from the caller to the callee stack.
 */
typedef struct {
	uval32 ins;
	sval (*hcall) ();
} hcall_vec_t;

/*
 * Scheduling parameters.
 * Describes the scheduling policy for this OS.
 * 64-bit bit-map for 64 scheduling slots.
 */
#define NUM_SCHED_SLOTS ((uval)(sizeof (uval64) * 8))

static inline struct tss *
get_cur_tss(void)
{
	struct dtr gdtr;
	uval32 tss_base;

	/* *INDENT-OFF* */
	__asm__ __volatile__ ("sgdt %0" : "=m" (gdtr) : );
	/* *INDENT-ON* */

	unpack_descr(&((struct descriptor *)gdtr.base)[__TSS >> 3],
		     NULL, &tss_base, NULL);
	return (struct tss *)tss_base;
}

/*
 * Per-OS data structure. This holds OS state (running, sleeping, etc.)
 * and priority information.
 *
 * Unless otherwise noted, all fields are guarded by po_mutex.
 */
struct os {
	struct hash_entry os_hashentry;
	struct cpu *cpu[MAX_CPU];
	int po_state;		/* OS state */
	uval16 po_lpid_tag;	/* OS index, not an ID */
	uval32 po_lpid;		/* OS ID */
	struct aipc_services aipc_srv;
	struct io_chan *po_chan[CHANNELS_PER_OS];
	uval po_vt[2];
	struct page_share_mgr *po_psm;
	uval16 installed_cpus;
	rw_lock_t po_mutex;	/* per-OS lock */
	struct pgcache pgcache;	/* page cache */
	uval32 iopl;		/* I/O privilege level */

	struct dlist po_resources;	/* Resources we have access to */

	struct os_events po_events;
	struct logical_memory_map logical_mmap;
	struct partition_info cached_partition_info[2];
	struct partition_info *os_partition_info;
};

/* called from generic code */
static inline uval
htab_init(uval eomem)
{
	return eomem;
}

extern struct cpu_thread *handle_external(struct cpu_thread *thread, uval srr0,
					  uval srr1);

/* values for the po_state field */
#define PO_STATE_EMPTY		0	/* Empty slot. */
#define PO_STATE_CREATED	1	/* Created, but not yet started. */
#define PO_STATE_RUNNABLE	2	/* Runnable. */
#define PO_STATE_BLOCKED	2	/* Blocked, e.g., wrong timeslot. */
#define PO_STATE_TERMINATED	4	/* Terminated, but status not */
					/*  collected. */
static inline uval
return_arg(struct cpu_thread *thread, uval argnum, uval arg)
{
	if (argnum < 6) {
		thread->tss.gprs.gprs[argnum] = arg;
	} else {
		uval daddr = thread->tss.gprs.regs.esp +
			(argnum - 6) * sizeof (uval);

		if (!put_lpar(thread, &arg, daddr, sizeof (uval))) {
			hprintf("Failed to return args!\n");
			return 0;
		}
	}

	return 1;
}

extern void hcall(struct cpu_thread *thread);
extern void resume_OS(struct cpu_thread *thread) __attribute__ ((noreturn));
extern int page_fault(struct cpu_thread *, uval32, uval);

extern uval hv_map_LA(struct os *os, uval laddr, uval size);

#endif /* __ASSEMBLY__ */

#endif /* __HYPE_X86_OS_H__ */
