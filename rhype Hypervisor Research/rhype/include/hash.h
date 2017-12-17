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
#ifndef _HASH_H
#define _HASH_H

#include <config.h>
#include <types.h>
#include <dlist.h>
#include <atomic.h>

#ifndef __ASSEMBLY__

struct hash_entry {
	uval he_key;
	struct hash_entry *he_next;
};

typedef uval (*hash_fn_t) (uval key, uval log_tbl_size);

enum {
	LOG_INIT_TBL_SIZE = 2,
	RESIZE_MULTIPLE = 4
};

struct hash_table {
	uval ht_count;
	struct hash_entry **ht_table;
	uval ht_log_tbl_size;
	hash_fn_t ht_hash;
	struct hash_entry *ht_init_table[1 << LOG_INIT_TBL_SIZE];
};

extern void ht_init(struct hash_table *ht, hash_fn_t func);

extern uval ht_insert(struct hash_table *ht, struct hash_entry *he);

extern struct hash_entry *ht_lookup(struct hash_table *ht, uval key);

extern struct hash_entry *ht_remove(struct hash_table *ht, uval key);

extern struct hash_entry *ht_pop(struct hash_table *ht);

/* Returns 0 on failure (hash table not empty) */
extern sval ht_destroy(struct hash_table *ht);

#endif /* ! __ASSEMBLY__ */

#endif /* _HASH_H */
