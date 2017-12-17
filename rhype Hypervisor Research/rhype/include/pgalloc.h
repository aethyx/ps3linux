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

#ifndef _PGALLOC_H
#define _PGALLOC_H

#include <config.h>
#include <types.h>

/* Page allocation must be done in blocks of x pages, where x is a
 * power of 2 and x is < # of bits in uval, or multiples of # of bits
 * in uval.
 */

struct pg_alloc {
	lock_t lock;
	struct lbm *lbm;
	uval base;
	uval log_pgsize;
};

#define PAGE_ALLOC_ERROR (~((uval)0))

extern uval pgalloc_init(struct pg_alloc *pa, uval bitmap_mem,
			 uval start_addr, uval length, uval log_pgsize);
extern uval get_pages(struct pg_alloc *pa, uval bytes);
extern uval get_zeroed_page(struct pg_alloc *pa);
extern void set_pages_used(struct pg_alloc *pa, uval page_addr, uval bytes);

/* this is equivalent to set_pages_free */
extern void free_pages(struct pg_alloc *pa, uval page_addr, uval bytes);

/* Log align is in terms of bytes, thus always >=12 (4096) */
extern uval get_pages_aligned(struct pg_alloc *pa, uval bytes, uval log_align);

#endif /* ! _PGALLOC_H */
