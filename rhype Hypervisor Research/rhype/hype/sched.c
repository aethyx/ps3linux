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
#include <lpar.h>
#include <lib.h>
#include <h_proto.h>
#include <hype.h>
#include <mmu.h>
#include <os.h>
#include <pmm.h>
#include <partition.h>
#include <sched.h>

static uval
match_slot(slot_mask_t fixed, slot_mask_t *orig)
{
	uval i = 0;
	slot_mask_t bits = *orig;

	while (i < NUM_SCHED_SLOTS) {
		/* Check if "bits" in "fixed" are unset */
		if ((bits & ~fixed) == bits) {
			break;
		}
		/* Rotate by one and try again */
		bits = (bits >> (NUM_SCHED_SLOTS - 1)) | (bits << 1ULL);
		++i;
	}

	if (i < NUM_SCHED_SLOTS) {
		/* i now tells us the rotation needed to make the mask fit */
		*orig = bits;
		return i;
	}

	return H_UNAVAIL;
}

/* Called with hypervisor hpc_mutex held */
static void
os_yield_slot(struct cpu *pcpu, uval slot_id)
{
	assert(pcpu != NULL, "no processor");
	pcpu->sched.curr_set &= ~(1ULL << slot_id);
	hype_per_cpu[(uval)pcpu->logical_cpu_num].hpc_sched.owners[slot_id] =
		NULL;
}

/* Called with hypervisor hpc_mutex held */
static void
os_acquire_slot(struct cpu *pcpu, uval slot_id, struct hype_scheduler *sched)
{
	sched->owners[slot_id] = pcpu;
	pcpu->sched.curr_set |= (1ULL << slot_id);
}

/* Called with hypervisor hpc_mutex held */
static void
set_slot_owners(short phys_cpu_num, slot_mask_t slots, struct cpu *pcpu)
{
	uval i = 0;

	while (i < NUM_SCHED_SLOTS) {
		struct hype_scheduler *hpcs;

		hpcs = &hype_per_cpu[phys_cpu_num].hpc_sched;

		if (slots & (0x1 << i) && hpcs->owners[i] != pcpu) {
			if (hpcs->owners[i])
				os_yield_slot(hpcs->owners[i], i);
			if (pcpu)
				os_acquire_slot(pcpu, i, hpcs);
		}
		++i;
	}
}

/* Locked version of h_set_sched_params.  Here "pcop" identifies the
 * per_cpu_os to be operated on, rather than the caller
 */
uval
locked_set_sched_params(struct cpu *pcpu, uval phys_cpu_num,
			slot_mask_t required, slot_mask_t desired)
{
	uval rotation;
	slot_mask_t tmp;
	slot_mask_t used = pcpu->sched.curr_set;
	struct hype_scheduler *hpcs = &hype_per_cpu[phys_cpu_num].hpc_sched;

	/* find match trying to not touch unlocked slots first */
	/* Make bitmask of bits we can consider as available for our use */
	/* We need to consider slots currently used by this OS as
	 * available for re-used */
	tmp = hpcs->used_slots & ~used;

	rotation = match_slot(tmp, &required);
	if (rotation >= NUM_SCHED_SLOTS) {
		/* Try again, but this time don't consider unlocked slots
		 * as invioable */
		tmp = hpcs->locked_slots & ~used;
		rotation = match_slot(tmp, &required);
	}

	if (rotation >= NUM_SCHED_SLOTS) {
		/* rotation of bitmask not possible, return (succes,
		 * but op failed)
		 */
		return NUM_SCHED_SLOTS;
	}

	/* Give up what we already have first */
	set_slot_owners(phys_cpu_num, used, NULL);

	set_slot_owners(phys_cpu_num, required, pcpu);

	desired = (desired >> (NUM_SCHED_SLOTS - rotation))
		| (desired << (rotation));

	/* Success is guaranteed regardless of requests for "desired" slots.
	 * So, record result of operation in pcop now. */

	/* Clear old settings */
	hpcs->locked_slots &= ~(used & pcpu->sched.required);
	hpcs->used_slots &= ~used;

	pcpu->sched.required = required;
	pcpu->sched.desired = desired;

	hpcs->locked_slots |= required;
	desired &= ~hpcs->locked_slots;

	set_slot_owners(phys_cpu_num, desired, pcpu);
	hpcs->used_slots |= (desired | required);

	return (H_Success);
}

/*
 * Tell scheduler it can run this partition
 */
uval
run_partition(struct cpu_thread *thread)
{
	thread->cpu->sched.runnable = 1;
	return 1;
}

void
run_cpu(struct cpu *pcpu)
{
	pcpu->sched.runnable = 1;
}

/*
 * Return pointer to next OS to be run on same processor as "current".
 * Returns NULL if nothing to be run.
 * Assumes hpc_mutex is held.
 */
struct cpu *
sched_next_os(struct cpu *cur_cpu)
{
	short logical_cpu_num;
	struct hype_scheduler *sched;
	uval slot;
	uval i = 0;
	struct cpu *next_cpu = NULL;
	uval slots_used = 0;

	assert(cur_cpu != NULL, "no current processor");

	logical_cpu_num = cur_cpu->logical_cpu_num;
	sched = &hype_per_cpu[logical_cpu_num].hpc_sched;
	slot = sched->curr_slot;

	slot = (slot + slots_used) % NUM_SCHED_SLOTS;

	/* directed yield */
	if (cur_cpu->sched.next_lpid != cur_cpu->os->po_lpid &&
	    cur_cpu->sched.next_lpid != 0) {
		/*
		 * We know which partition to switch to. We're only
		 * yielding the rest of the current time slot
		 */
		struct os *next_os = os_lookup(cur_cpu->sched.next_lpid);

		if (next_os) {
			next_cpu = next_os->cpu[logical_cpu_num];
			return next_cpu;
		}
	}

	for (i = 0; i < NUM_SCHED_SLOTS; ++i) {
		slot = (slot + 1) % NUM_SCHED_SLOTS;
		next_cpu = sched->owners[slot];
		if (next_cpu == NULL) {
			continue;
		} else if (next_cpu->sched.runnable) {
			break;
		}
	}

	if ((next_cpu == NULL) || (!next_cpu->sched.runnable)) {
		return cur_cpu;
	}

	sched->curr_slot = slot;

	return next_cpu;
}
