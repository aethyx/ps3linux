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

#ifndef _HYP_RESOURCE_H
#define _HYP_RESOURCE_H
/*
 * Resource management.
 *
 */
#include <config.h>
#include <types.h>
#include <lib.h>
#include <hcall.h>
#include <hash.h>
#include <rcu.h>

extern lock_t sys_resource_lock;
extern struct hash_table sys_resource_hash;

enum {
	RES_MAPPED = 0x1,
	RES_RESCINDED = 0x2,
};

/* When adding these object to any dlist, always use
 * dlist_ins_before(sys_resource, list_head)
 */
struct sys_resource {
	uval sr_cookie;
	lock_t sr_lock;
	uval32 sr_type;		/* from h_grant_logical "flags" arg */
	uval32 sr_flags;	/* from h_grant_logical "flags" arg */
	struct os *sr_owner;
	struct sys_resource *sr_parent;

	struct dlist sr_os_res;	/* List maintained by LPAR */
	/* Protect with os->po_mutex lock */

	struct dlist sr_siblings;	/* Protect with parent's sr_lock */

	struct dlist sr_children;	/* Protect with sr_lock */
};

struct resource_action {
	sval (*grant_fn) (struct sys_resource ** res, struct os * src,
			  uval flags, uval logical_hi, uval logical_lo,
			  uval size, struct os * dest);
	sval (*accept_fn) (struct sys_resource * res, uval *retval);

	/* Called when RES_RESCINDED is about to be asserted and
	 * RES_MAPPED is asserted */
	/* Is to ensure that no further hcalls that use this resource
	 * are to succeed */
	sval (*invalidate_fn) (struct sys_resource * res);

	/* Unmaps the resource from the owner's space, but does not
	 * disassociate the resource from the partition */
	sval (*return_fn) (struct sys_resource * res);

	/* This function is expected to destroy the res object using
	 * RCU.  References to "res" may be made after this call, but
	 * sr_parent will be RESCINDED_RESOURCE and thus all
	 * operations will fail.  RCU mechanisms may the subsequently
	 * delete res.  (Since grant_fn allocates res, we must permit
	 * a resource-specific function to define the RCU desctructor
	 * to be associated with it.) */
	sval (*rescind_fn) (struct sys_resource * res);

	uval (*conflict_fn) (struct sys_resource * res1,
			     struct sys_resource * res2);
};

extern struct resource_action *res_actions[];

/* Check if os already has access to sr */
extern sval resource_conflict(struct os *os, struct sys_resource *sr);

extern void resource_init(struct sys_resource *sr,
			  struct sys_resource *parent, uval type);

extern sval get_sys_resource(struct os *os,
			     uval cookie, struct sys_resource **sr);

extern sval grant_resource(struct sys_resource **sr, struct os *src_os,
			   uval flags,
			   uval logical_hi, uval logical_lo,
			   uval length, struct os *dest_os);

extern sval resource_transfer(struct cpu_thread *thread, uval type,
			      uval addr_hi, uval addr_lo, uval size,
			      struct os *target_os);

/* Associates a resource with a partition */
extern sval insert_resource(struct sys_resource *sr, struct os *os);

/* Associates a resource with logical addresses */
/* sr is assumed to be locked */
extern sval accept_locked_resource(struct sys_resource *sr, uval *retval);

extern sval partition_death_cleanup(struct os *os);

extern sval force_rescind_resource(struct sys_resource *sr /* locked */ );

extern sval forced_rescind_cleanup(struct dlist *nodes);
extern sval forced_rescind_mark(struct sys_resource *sr,
				struct dlist *nodes, struct os *avoid);

#endif /* _HYP_RESOURCE_H */
