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
 * Old-style doubly-linked-list manipulation macros.
 */

#ifndef _DLIST_H
#define _DLIST_H

#include <config.h>

#ifndef __ASSEMBLY__

struct dlist {
	struct dlist *dl_next;
	struct dlist *dl_prev;
};

static inline void
dlist_init(struct dlist *elem)
{
	elem->dl_next = elem;
	elem->dl_prev = elem;
}

/* Appends "item" to end of "to"  (i.e. inserts it to to->dl_next) */
static inline void
dlist_ins_after(struct dlist *item, struct dlist *to)
{
	struct dlist *last = item->dl_prev;
	struct dlist *first = item;
	struct dlist *before = to;
	struct dlist *after = to->dl_next;

	last->dl_next = after;
	after->dl_prev = last;

	before->dl_next = first;
	first->dl_prev = before;
}

/* Prepends "item" in front of "to" (i.e. inserts item->dl_prev to
 * to->dl_prev) */
static inline void
dlist_ins_before(struct dlist *item, struct dlist *to)
{
	struct dlist *last = item->dl_prev;
	struct dlist *first = item;
	struct dlist *before = to->dl_prev;
	struct dlist *after = to;

	last->dl_next = after;
	after->dl_prev = last;

	before->dl_next = first;
	first->dl_prev = before;
}

/* If you don't care about order */
static inline void
dlist_insert(struct dlist *item, struct dlist *list)
{
	dlist_ins_before(item, list);
}

static inline void
dlist_detach(struct dlist *item)
{
	struct dlist *before = item->dl_prev;
	struct dlist *after = item->dl_next;

	before->dl_next = after;
	after->dl_prev = before;
}

static inline struct dlist *
dlist_prev(struct dlist *elem)
{
	return elem->dl_prev;
}

static inline struct dlist *
dlist_next(struct dlist *elem)
{
	return elem->dl_next;
}

static inline uval
dlist_empty(struct dlist *item)
{
	return item->dl_next == item;
}

#endif /* ! __ASSEMBLY__ */

#endif /* ! _DLIST_H */
