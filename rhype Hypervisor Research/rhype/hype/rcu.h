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
#ifndef _RCU_H
#define _RCU_H

#include <types.h>
#include <config.h>
#include <dlist.h>

struct rcu_node;
typedef void (*collector_fn) (struct rcu_node *);

struct rcu_node {
	struct rcu_node *next;
	collector_fn rcu_collector;
};

#define RCU_PARENT(type, member, rcu)				\
	((type*)(((uval)rcu) - ((uval)&((type*)NULL)->member)))

extern void rcu_check(void);
extern void rcu_init(void);
extern void rcu_defer(struct rcu_node *rcu, collector_fn collector);

/* Wait until it a garbage collection has occurred.
 * Cannot hold any locks when calling this.
 * Cannot hold pointers which may change due to deferred RCU actions running.
 */
extern void rcu_pause(void);

#endif /* ! _RCU_H */
