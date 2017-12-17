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
#include <hype.h>
#include <types.h>
#include <lib.h>
#include <hash.h>
#include <resource.h>
#include <objalloc.h>
#include <os.h>

static volatile uval keys = 1;

extern struct resource_action mem_actions;
extern struct resource_action mmio_actions;
extern struct resource_action vio_actions;

struct resource_action *res_actions[MAX_TYPE + 1] = {
	[MEM_ADDR] = &mem_actions,
	[MMIO_ADDR] = &mmio_actions,
	[INTR_SRC] = &vio_actions,
};

sval
get_sys_resource(struct os *os, uval cookie, struct sys_resource **sr)
{
	read_lock_acquire(&os->po_mutex);
	struct dlist *curr = dlist_next(&os->po_resources);

	while (curr != &os->po_resources) {
		*sr = PARENT_OBJ(typeof(**sr), sr_os_res, curr);

		if ((*sr)->sr_cookie == cookie) {

			read_lock_release(&os->po_mutex);

			if ((*sr)->sr_flags & RES_RESCINDED) {
				return H_Rescinded;
			}

			return H_Success;
		}
		curr = dlist_next(curr);
	}

	read_lock_release(&os->po_mutex);
	return H_Parameter;
}

void
resource_init(struct sys_resource *sr, struct sys_resource *parent, uval type)
{
	sr->sr_cookie = 0;
	sr->sr_type = type;
	sr->sr_flags = 0;
	sr->sr_parent = parent;
	dlist_init(&sr->sr_siblings);
	dlist_init(&sr->sr_children);
	dlist_init(&sr->sr_os_res);
	lock_init(&sr->sr_lock);
}

sval
resource_conflict(struct os *os, struct sys_resource *sr)
{
	struct dlist *curr;
	sval err = H_Success;

	read_lock_acquire(&os->po_mutex);

	curr = dlist_next(&os->po_resources);
	while (curr != &os->po_resources) {
		struct sys_resource *res = PARENT_OBJ(typeof(*sr),
						      sr_os_res, curr);

		if (!(res->sr_flags & RES_RESCINDED) &&
		    res->sr_type == sr->sr_type &&
		    res_actions[sr->sr_type]->conflict_fn(res, sr)) {
			err = H_UNAVAIL;
			break;
		}
		curr = dlist_next(curr);
	}

	read_lock_release(&os->po_mutex);
	return err;
}

sval
insert_resource(struct sys_resource *res, struct os *os)
{
	/* Lock acquisition is always parents first, children second */
	if (res->sr_parent) {
		lock_acquire(&res->sr_parent->sr_lock);
	}
	lock_acquire(&res->sr_lock);

	if (res->sr_parent && res->sr_parent->sr_flags & RES_RESCINDED) {
		lock_release(&res->sr_parent->sr_lock);
		lock_release(&res->sr_lock);
		return H_Rescinded;
	}

	if (res->sr_parent) {
		dlist_ins_before(&res->sr_siblings,
				 &res->sr_parent->sr_children);
		lock_release(&res->sr_parent->sr_lock);
	}

	sval cookie = (sval)atomic_add((volatile uval *)&keys, 1);

	res->sr_cookie = cookie;

	lock_release(&res->sr_lock);
	/* Need po_mutex lock, not sr_lock for association with os */
	write_lock_acquire(&os->po_mutex);

	res->sr_owner = os;
	dlist_ins_before(&res->sr_os_res, &os->po_resources);
	write_lock_release(&os->po_mutex);

	return cookie;
}

sval
accept_locked_resource(struct sys_resource *sr, uval *retval)
{
	sval err;

	err = (*res_actions[sr->sr_type]->accept_fn) (sr, retval);

	if (err == H_Success) {
		sr->sr_flags |= RES_MAPPED;
	}
	return err;
}

sval
grant_resource(struct sys_resource **sr, struct os *src_os, uval flags,
	       uval logical_hi, uval logical_lo,
	       uval length, struct os *dest_os)
{
	sval err = H_Success;

	if (!(flags < ARRAY_SIZE(res_actions) && res_actions[flags]->grant_fn)) {
		return H_UNAVAIL;
	}

	struct sys_resource *res;

	err = (*res_actions[flags]->grant_fn) (&res, src_os, flags,
					       logical_hi, logical_lo,
					       length, dest_os);

	if (err != H_Success)
		return err;

	if (res->sr_flags & RES_RESCINDED) {
		err = H_Rescinded;
		(*res_actions[flags]->rescind_fn) (res);
		return err;
	}

	err = insert_resource(res, dest_os);

	if (err > 0) {
		*sr = res;
	} else {
		(*res_actions[flags]->rescind_fn) (res);
	}

	return err;
}

static sval
mark_for_rescind(struct sys_resource *sr)
{
	do {
		uval old = sr->sr_flags;

		if (old & RES_RESCINDED) {
			return 0;
		}
		if (cas_uval32(&sr->sr_flags, old, old | RES_RESCINDED)) {
			return 1;
		}

	} while (1);
	return 0;
}

/* Mark a resource and all of it's children as being rescinded */
sval
forced_rescind_mark(struct sys_resource *start,
		    struct dlist *nodes, struct os *avoid)
{

	/* If avoid is non-NULL, then when we encounter any resources
	 * that are owned by that partition, we treat them as if
	 * somebody else is already rescinding them.  See
	 * partition_death_cleanup()
	 */

	/* DANGER!!!
	 * DANGER!!!
	 *
	 * There be dragons here.
	 *
	 * There is a race we cannot handle; partitions
	 * may be performing loops grant/accept and growing the
	 * cookie-tree we are trying to rescind.  We anticipate that we
	 * would catch up to those operations and eventually win the race
	 * by locking the leaves and rescinding them.
	 */

	/* sr is locked --> sr_parent field can't change */
	struct sys_resource *sr = start;

	if (!mark_for_rescind(sr)) {
		return H_Rescinded;
	}
	struct sys_resource *parent = sr->sr_parent;

	if (parent) {
		lock_acquire(&parent->sr_lock);
		dlist_detach(&start->sr_siblings);
		dlist_init(&start->sr_siblings);
		lock_release(&parent->sr_lock);
	}

	lock_acquire(&sr->sr_lock);

	struct resource_action *ra = res_actions[sr->sr_type];

	if (sr->sr_flags & RES_MAPPED) {
		(*ra->invalidate_fn) (sr);
	}

	struct dlist *curr;

	/* Add to the end of the list.  This preserves ordering
	 * invariant - all children follow any potential parents.
	 */
	dlist_ins_before(&sr->sr_siblings, nodes);
	curr = &sr->sr_siblings;
	lock_release(&sr->sr_lock);

	/* We do a breadth-first walk of the tree, by placing all
	 * nodes we encounter on the end of the "nodes" list. */

	while (curr != nodes) {
		sr = PARENT_OBJ(typeof(*sr), sr_siblings, curr);
		lock_acquire(&sr->sr_lock);

		while (!dlist_empty(&sr->sr_children)) {
			struct dlist *x = dlist_next(&sr->sr_children);
			struct sys_resource *child;

			child = PARENT_OBJ(typeof(*child), sr_siblings, x);

			child->sr_parent = NULL;
			dlist_detach(x);
			dlist_init(x);

			/* Ignore children we're not supposed to handle */
			if (child->sr_owner == avoid) {
				continue;
			}

			/* We know that children may be being
			 * rescinded on their own.  And we simply let
			 * that continue.  An argument can be made
			 * that such a rescind is guaranteed to be
			 * complete only after the RCU gen count
			 * update that we use to clean up our rescind.
			 * Thus it is only seen as being finished when
			 * our rescind is finished. If that sounds
			 * fishy, it probably is.
			 */

			if (!mark_for_rescind(child)) {
				continue;
			}
			lock_acquire(&child->sr_lock);

			if (child->sr_flags & RES_MAPPED) {
				(*ra->invalidate_fn) (child);
			}

			/* Order is important here, needs to go
			 * on end of list */
			dlist_ins_before(&child->sr_siblings, nodes);

			lock_release(&child->sr_lock);
		}
		lock_release(&sr->sr_lock);
		curr = dlist_next(curr);
	}

	return 0;
}

/* This is called after the quiescent period, after the cookies tree
 * has been marked for rescind. */
sval
forced_rescind_cleanup(struct dlist *nodes)
{

	/* We have marked all nodes in the tree, and all nodes x
	 * levels from the root are before all nodes x+1 levels from
	 * the root.  We walk the tree backwards.  Thus when a node is
	 * processed here, we know that all of it's children have been
	 * processed already as well. */

	struct dlist *curr;
	struct sys_resource *sr;

	curr = dlist_prev(nodes);
	while (curr != nodes) {
		sr = PARENT_OBJ(typeof(*sr), sr_siblings, curr);
		curr = dlist_prev(curr);

		write_lock_acquire(&sr->sr_owner->po_mutex);
		dlist_detach(&sr->sr_os_res);
		write_lock_release(&sr->sr_owner->po_mutex);

		lock_acquire(&sr->sr_lock);

		assert(sr->sr_flags & RES_RESCINDED,
		       "node not rescind marked\n");
		assert(dlist_empty(&sr->sr_children), "still have children\n");

		/* Unmap the resource */
		if (sr->sr_flags & RES_MAPPED) {
			(*res_actions[sr->sr_type]->return_fn) (sr);
		}

		(*res_actions[sr->sr_type]->rescind_fn) (sr);

		dlist_detach(&sr->sr_siblings);
		lock_release(&sr->sr_lock);
	}
	return 0;

}

struct rescind_rcu {
	struct rcu_node rcu;
	struct dlist todo;
};

static void
rescind_rcu_cleanup(struct rcu_node *rcu)
{
	struct rescind_rcu *rr = PARENT_OBJ(struct rescind_rcu, rcu, rcu);

	forced_rescind_cleanup(&rr->todo);
	hfree(rr, sizeof (struct rescind_rcu));
}

sval
force_rescind_resource(struct sys_resource *sr)
{
	struct rescind_rcu *rr = halloc(sizeof (*rr));

	dlist_init(&rr->todo);
	sval err = forced_rescind_mark(sr, &rr->todo, NULL);

	if (err == H_Rescinded) {
		return H_Success;
	}

	assert(err == H_Success, "Rescind failure !?!?!?");

	/* Wait for any pending hcalls to flush out and continue the
	 * rest of the cleanup.
	 */

	rcu_defer(&rr->rcu, rescind_rcu_cleanup);

	return err;
}

/*
 * Partition clean-up
 */

struct death_rcu {
	struct rcu_node rcu;
	struct dlist todo;
	struct os *os;
};

static void
death_rcu_cleanup(struct rcu_node *rcu)
{
	struct death_rcu *dr = PARENT_OBJ(struct death_rcu, rcu, rcu);

	forced_rescind_cleanup(&dr->todo);

	arch_os_destroy(dr->os);

	os_free(dr->os);
	hfree(dr, sizeof (struct death_rcu));
}

sval
partition_death_cleanup(struct os *os)
{
	struct dlist *curr;

	struct death_rcu *dr = halloc(sizeof (*dr));

	dlist_init(&dr->todo);
	dr->os = os;

	/* The order of lock acquisition is sr_lock, followed by
	 * po_mutex.  There may be other threads trying to acquire
	 * locks in this order (forced_rescind_cleanup), so we must
	 * follow those semantics.  Thus we take sr's off the
	 * po_resources list, release po_mutex and grab sr_lock.  This
	 * is safe since we use RCU to deallocate sr.
	 */
	write_lock_acquire(&os->po_mutex);
	curr = dlist_next(&os->po_resources);

	while (curr != &os->po_resources) {
		struct dlist *next;
		struct sys_resource *sr;

		sr = PARENT_OBJ(typeof(*sr), sr_os_res, curr);

		next = dlist_next(curr);
		dlist_detach(curr);
		if (sr->sr_flags & RES_RESCINDED) {
			/* It's being handled already */
			curr = next;
			continue;
		}
		write_lock_release(&os->po_mutex);

		/* Using the avoid argument ensures that none of the
		 * items in this list are added before being processed
		 * here.
		 */
		forced_rescind_mark(sr, &dr->todo, os);

		write_lock_acquire(&os->po_mutex);
		curr = dlist_next(&os->po_resources);
	}
	write_lock_release(&os->po_mutex);

	/* Wait for any in-progress hcalls to get out and continue the
	 * rest of the cleanup.
	 */
	rcu_defer(&dr->rcu, death_rcu_cleanup);

	return 0;

}

sval
resource_transfer(struct cpu_thread *thread, uval type,
		  uval addr_hi, uval addr_lo, uval size,
		  struct os *target_os)
{
	struct sys_resource *sr;
	struct os *os = thread->cpu->os;
	uval ret;
	sval rc;

	if (!(type  & (MEM_ADDR | MMIO_ADDR | INTR_SRC))) {
		return H_Parameter;
	}

	rc = grant_resource(&sr, os, type,
			     addr_hi, addr_lo, size, target_os);
	if (rc >= 0) {
		lock_acquire(&sr->sr_lock);
		rc = accept_locked_resource(sr, &ret);
		lock_release(&sr->sr_lock);

		if (rc == H_Success) {

#ifdef DEBUG
			hprintf("%s: assigned: 0x%lx[%lx] to LPAR[0x%x]\n",
				__func__, ret, size, target_os->po_lpid);
#endif

			return_arg(thread, 1, ret);
			return H_Success;
		}
		/* grant it back */
		rc = grant_resource(&sr, target_os, type,
				    addr_hi, addr_lo, size, os);
		assert(rc >= 0, "regranting failed\n");
	}
	/* shouldn't we return rc here? */
	return H_Parameter;
}
