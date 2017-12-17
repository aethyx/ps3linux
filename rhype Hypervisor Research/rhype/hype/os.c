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
 * Allocate and deallocate OS-related data structures.
 * This is globally locked -- if you are creating and destroying
 * OSes quickly enough to stress this lock, you have many other
 * more serious problems...
 *
 */

#include <config.h>
#include <hypervisor.h>
#include <hype.h>
#include <float.h>
#include <pmm.h>
#include <partition.h>
#include <mmu.h>
#include <thread_control_area.h>
#include <thinwire.h>
#include <psm.h>
#include <timer.h>
#include <xir.h>
#include <objalloc.h>

static struct hash_table os_hash;
struct hype_per_cpu_s hype_per_cpu[MAX_CPU];
uval cur_lpid = 0x1000;

/*
 * Allocate a single per_cpu_os structure, returning NULL if none available.
 * Assumes lock already held.
 */
static struct cpu *
cpu_create(struct os *os, uval cpu_idx)
{
	int i;
	struct cpu *pcpu;

	pcpu = halloc(sizeof(*pcpu));
	if (pcpu == NULL)
		return NULL;
	memset(pcpu, 0, sizeof (*pcpu));

	pcpu->os = os;
	pcpu->logical_cpu_num = cpu_idx;

	for (i = 0; i < THREADS_PER_CPU; ++i) {
		const uval32 thrid =
			(((('T' << 16) | 'H' << 16) | 'R' << 16) | '0' << 16);
		pcpu->thread[i].cpu = pcpu;
		pcpu->thread[i].thr_num = (uval8)i;
		pcpu->thread[i].cpu_num = (uval8)cpu_idx;
		pcpu->thread[i].eyecatch = thrid + i;	/* "THRi" */
		lock_init(&pcpu->thread[i].lock);
	}

	pcpu->eyecatch = (((('C' << 16) | 'P' << 16) | 'U' << 16) | '0' << 16);

	return pcpu;
}

static void
cpu_free(struct cpu *pcpu)
{
	hfree(pcpu, sizeof(*pcpu));
}

/*
 * Allocate the data structures required for an OS.  Returns a pointer
 * to the os structure.
 */

static struct os *
os_alloc(int n_cpus)
{
	struct os *os;
	int i;

	os = halloc(sizeof(*os));
	if (os == NULL)
		return NULL;
	memset(os, 0, sizeof (*os));

	for (i = 0; i < n_cpus; i++) {
		os->cpu[i] = cpu_create(os, i);
		if (os->cpu[i] == NULL)
			goto unwind;

		os->installed_cpus++;
	}

	return os;
/* *INDENT-OFF* */
unwind:
/* *INDENT-ON* */
	if (os != NULL)
		os_free(os);

	return NULL;
}

/*
 * Free up an OS data structure allocated by os_alloc().
 */

void
os_free(struct os *os)
{
	int i;

#ifdef HAS_HTAB
	if (!os->cached_partition_info[1].sfw_tlb) {
		htab_free(os);
	}
#endif /* HAS_HTAB */

	if (os->po_psm)
		os_psm_destroy(os->po_psm);

	for (i = 0; i < os->installed_cpus; i++) {
		cpu_free(os->cpu[i]);
		os->cpu[i] = NULL;
	}

	ht_remove(&os_hash, os->po_lpid);

	os->po_lpid = -1;

	hfree(os, sizeof(*os));
}

/*
 * Create an OS
 */

struct os *
os_create(void)
{
	struct os *newOS;

	newOS = os_alloc(MAX_CPU);
	if (newOS == NULL)
		return NULL;

	rw_lock_init(&newOS->po_mutex);
	logical_mmap_init(newOS);
	newOS->po_state = PO_STATE_CREATED;
	newOS->po_lpid = atomic_add(&cur_lpid, 1);

	/* add to global OS hash */
	lock_acquire(&hype_mutex);
	newOS->os_hashentry.he_key = newOS->po_lpid;
	ht_insert(&os_hash, &newOS->os_hashentry);
	lock_release(&hype_mutex);

	lock_init(&newOS->po_events.oe_lock);
	dlist_init(&newOS->po_events.oe_list);
	dlist_init(&newOS->po_resources);

	return newOS;
}

struct os *
os_lookup(uval lpid)
{
	struct hash_entry *entry;
	struct os *found = NULL;

	if (lpid == (uval)H_SELF_LPID) {
		return get_cur_thread()->cpu->os;
	}

	lock_acquire(&hype_mutex);
	entry = ht_lookup(&os_hash, lpid);
	lock_release(&hype_mutex);
	if (entry)
		found = PARENT_OBJ(struct os, os_hashentry, entry);

	return found;
}

sval
register_event(struct os *os, struct lpar_event *le)
{
	dlist_init(&le->le_list);
	lock_acquire(&os->po_events.oe_lock);
	dlist_insert(&os->po_events.oe_list, &le->le_list);
	lock_release(&os->po_events.oe_lock);
	return 0;
}

sval
unregister_event(struct os *os, struct lpar_event *le)
{
	lock_acquire(&os->po_events.oe_lock);
	dlist_detach(&le->le_list);
	lock_release(&os->po_events.oe_lock);
	return 0;
}

void
run_event(struct os *os, uval event)
{
	struct dlist *curr = dlist_next(&os->po_events.oe_list);

	lock_acquire(&os->po_events.oe_lock);
	while (curr != &os->po_events.oe_list) {
		struct lpar_event *le = PARENT_OBJ(typeof(*le), le_list, curr);

		dlist_detach(&le->le_list);
		curr = dlist_next(curr);

		uval ret = (*le->le_func) (os, le, event);

		/*
		 * ret == 1 || event==LPAR_DIE --> le is now a bad pointer,
		 * memory may have been freed
		 */
		if (ret == 0 && (event != LPAR_DIE)) {
			dlist_insert(curr, &le->le_list);
		}

	}
	lock_release(&os->po_events.oe_lock);
}

static uval
os_hashfunc(uval lpid, uval log_tbl_size)
{
	(void)log_tbl_size;
	return lpid % 13; /* whatever... */
}

void
os_init(void)
{
	ht_init(&os_hash, &os_hashfunc);
}
