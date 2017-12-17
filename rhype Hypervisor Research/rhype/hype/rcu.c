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
#include <types.h>
#include <rcu.h>
#include <dlist.h>
#include <thread_control_area.h>

enum { NUM_GEN = 4 };
static struct rcu_node *rcu_lists[NUM_GEN];
static struct rcu_node **rcu_tails[NUM_GEN];
static lock_t rcu_lock;
static volatile uval gen_count = 0;
static uval cpu_mask = 0;
static volatile uval gen_mask = 0;

static volatile uval inactive_mask = 0;

void
rcu_pause(void)
{
	uval mybit = (((uval)1) << get_tca_index());
	uval old;

	uval start_count = gen_count;

	do {
		old = inactive_mask;
	} while (!cas_uval(&inactive_mask, old, old | mybit));

	do {
		rcu_check();
	} while (gen_count == start_count);

	do {
		old = inactive_mask;
	} while (!cas_uval(&inactive_mask, old, old & ~mybit));

}

void
rcu_init(void)
{
	if (cpu_mask == 0) {
		int i = 0;

		for (; i < NUM_GEN; ++i) {
			rcu_lists[i] = NULL;
			rcu_tails[i] = &rcu_lists[i];
		}
	}
	cpu_mask |= ((uval)1) << get_tca_index();
}

void
rcu_check(void)
{
	uval new_gen;
	uval old;
	uval bit = (((uval)1) << get_tca_index());
	uval do_cleanup;
	uval repeat = 0;

/* *INDENT-OFF* */
retry:
/* *INDENT-ON* */

	do_cleanup = 0;
	do {
		old = gen_mask;
		new_gen = old | bit;
		if (new_gen == cpu_mask) {
			/* ensure bit for this cpu is not set,
			 * this prevents another cpu from doing a list scan */
			new_gen = (inactive_mask & ~bit);
			do_cleanup = 1;
		}
	} while (!cas_uval(&gen_mask, old, new_gen));

	if (do_cleanup) {
		lock_acquire(&rcu_lock);

		struct rcu_node *list = rcu_lists[gen_count % NUM_GEN];
		struct rcu_node *next;

		rcu_lists[gen_count % NUM_GEN] = NULL;
		rcu_tails[gen_count % NUM_GEN] =
			&rcu_lists[gen_count % NUM_GEN];

		++gen_count;
		lock_release(&rcu_lock);

		while (list) {
			next = list->next;

			(*list->rcu_collector) (list);

			list = next;
		}

		++repeat;
		/* We're at a safe-point.  If everyone else is ok with
		 * it, let's just keep on cleaning up.  This primarily
		 * affects the single CPU case, assuring we clean up
		 * everything.
		 */
		if ((repeat < NUM_GEN) && ((bit | gen_mask) == cpu_mask)) {
			goto retry;
		}
	}

	/* We may have been inactive */
	if ((inactive_mask & bit) && do_cleanup) {
		/* And thus avoided setting our bit */
		while (!cas_uval(&gen_mask, gen_mask, gen_mask | bit)) ;

		/* So now check if that may have prevented another cleanup */
		if (gen_mask == cpu_mask && ((new_gen | bit) != cpu_mask)) {
			goto retry;
		}
	}

}

void
rcu_defer(struct rcu_node *rcu, collector_fn collector)
{

	/* We have to use a lock so as to preserve ordering of items
	 * in the list, so that we process things FIFO
	 */

	lock_acquire(&rcu_lock);

	uval gen = (gen_count + 2) % NUM_GEN;

	rcu->rcu_collector = collector;
	rcu->next = NULL;
	*rcu_tails[gen] = rcu;
	rcu_tails[gen] = &rcu->next;

	lock_release(&rcu_lock);

}
