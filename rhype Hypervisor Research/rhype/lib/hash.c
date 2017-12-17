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
 * Hash-table implementation.
 */
#include <config.h>
#include <lib.h>
#include <atomic.h>
#include <objalloc.h>
#include <hash.h>

static void __ht_grow(struct hash_table *ht);

static uval default_hash(uval key, uval log_tbl_size)
{
	return key & ((1UL << log_tbl_size) - 1);
}

void ht_init(struct hash_table* ht, hash_fn_t func)
{
	if (func == NULL) {
		func = &default_hash;
	}
	ht->ht_hash = func;
	ht->ht_table = (struct hash_entry**)&ht->ht_init_table;
	ht->ht_log_tbl_size = LOG_INIT_TBL_SIZE;
	ht->ht_count = 0;
	uval i;
	for (i = 0; i < (1UL<<ht->ht_log_tbl_size); ++i) {
		ht->ht_table[i] = NULL;
	}
}

static inline uval
__ht_insert(struct hash_table* ht, struct hash_entry *he)
{
	uval hash = ht->ht_hash(he->he_key, ht->ht_log_tbl_size);
	hash &= (1UL<<ht->ht_log_tbl_size) - 1;
	he->he_next = ht->ht_table[hash];
	ht->ht_table[hash] = he;
	++ht->ht_count;
	return 0;
}

uval
ht_insert(struct hash_table* ht, struct hash_entry *he)
{
	uval ret;
	ret = __ht_insert(ht, he);

	if (ht->ht_count >= RESIZE_MULTIPLE * (1ULL << ht->ht_log_tbl_size)) {
		__ht_grow(ht);
	}

	return ret;
}

struct hash_entry*
ht_lookup(struct hash_table *ht, uval key)
{
	uval hash = ht->ht_hash(key, ht->ht_log_tbl_size);
	hash &= (1UL<<ht->ht_log_tbl_size) - 1;
	struct hash_entry* he = ht->ht_table[hash];
	while (he && he->he_key != key) {
		he = he->he_next;
	}
	return he;
}

struct hash_entry*
ht_remove(struct hash_table *ht, uval key)
{
	uval hash = ht->ht_hash(key, ht->ht_log_tbl_size);
	hash &= (1UL<<ht->ht_log_tbl_size) - 1;

	struct hash_entry* he = ht->ht_table[hash];
	struct hash_entry** phe = &ht->ht_table[hash];
	while (he && he->he_key != key) {
		phe = &he->he_next;
		he = he->he_next;
	}

	if (he) {
		*phe = he->he_next;
		he->he_next = NULL;
	}
	--ht->ht_count;
	return he;

}

void
__ht_grow(struct hash_table *ht)
{
	uval do_free = 1;

	if (ht->ht_table == &ht->ht_init_table[0]) {
		do_free = 1;
	}

	struct hash_entry ** old = ht->ht_table;
	uval old_size = 1ULL << ht->ht_log_tbl_size;
	++ht->ht_log_tbl_size;
	ht->ht_table = halloc(sizeof(struct hash_entry*) *
			      (1ULL << ht->ht_log_tbl_size));

	uval i = 0;
	while (i < old_size) {
		while (old[i]) {
			struct hash_entry* curr = old[i];
			old[i] = curr->he_next;
			--ht->ht_count;
			__ht_insert(ht, curr);
		}
		++i;

	}

	hfree(old, old_size);
}

struct hash_entry*
ht_pop(struct hash_table *ht)
{
	if (ht->ht_count == 0) return NULL;
	uval i = 0;
	while (i < (1UL<<ht->ht_log_tbl_size)) {
		if (ht->ht_table[i] != NULL) {
			return ht_remove(ht, ht->ht_table[i]->he_key);
		}
		++i;
	}
	return NULL;
}

sval
ht_destroy(struct hash_table *ht)
{
	if (ht->ht_count) return ht->ht_count;

	hfree(ht->ht_table, 1ULL << ht->ht_log_tbl_size);

	return 0;
}
