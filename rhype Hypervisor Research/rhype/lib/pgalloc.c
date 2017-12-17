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
 *
 */
#include <config.h>
#include <types.h>
#include <lib.h>
#include <mmu.h>
#include <atomic.h>
#include <util.h>
#include <bitmap.h>
#include <pgalloc.h>

#define UVAL_BITS (sizeof(uval) * 8)

uval pgalloc_debug = 0;
#define debug(FMT...) if (pgalloc_debug) hprintf(FMT);

uval
pgalloc_init(struct pg_alloc* pa, uval lbm_addr,
	     uval start_addr, uval length, uval log_page_size)
{
	uval page_size = 1 << log_page_size;
	lock_init(&pa->lock);

	if (ALIGN_DOWN(start_addr, page_size) != start_addr) {
		uval orig_start_addr = start_addr;
		start_addr = ALIGN_UP(start_addr, page_size);
		length -= (start_addr - orig_start_addr);
	}

	pa->base = start_addr >> log_page_size;

	length = ALIGN_DOWN(length, page_size);
	uval num_pages = length >> log_page_size;
	pa->log_pgsize = log_page_size;

	if (lbm_addr == ~((uval)0)) {
		pa->lbm = lbm_alloc(ALIGN_UP(num_pages, UVAL_BITS));
		lbm_set_all_bits(pa->lbm, 1);
	} else {
		uval bits = ALIGN_UP(num_pages, UVAL_BITS);
		uval lbm_size = sizeof(struct lbm) + (bits / 8);
		lbm_size = ALIGN_UP(lbm_size, PGSIZE);
		lbm_addr = ALIGN_UP(lbm_addr, PGSIZE);
		pa->lbm = lbm_init(bits, 0, (void*)lbm_addr);

		lbm_set_all_bits(pa->lbm, 1);
		lbm_set_bits(pa->lbm, 0,
			     (lbm_addr + lbm_size - start_addr)/page_size, 0);
//		lbm_set_bits(pa->lbm, (lbm_addr + lbm_size)/page_size,
//			     num_pages, 1);
	}

	/*
	 * Mark as 0 any extra bits at the end
	 */
	lbm_set_bits(pa->lbm, num_pages,
		     UVAL_BITS - (num_pages % UVAL_BITS), 0);
	return 0;
}

static inline void
__locked_set_page_bits(struct pg_alloc *pa, uval page_addr,
		       uval bytes, uval setbit)
{

        uval page_num = (page_addr >> pa->log_pgsize) - pa->base;
	uval num_pages= ALIGN_UP(bytes, 1 << pa->log_pgsize) >> pa->log_pgsize;
	lbm_set_bits(pa->lbm, page_num, num_pages, setbit);
}

void
set_pages_used(struct pg_alloc *pa, uval page_addr, uval bytes)
{
	lock_acquire(&pa->lock);
	__locked_set_page_bits(pa, page_addr, bytes, 0);
	lock_release(&pa->lock);
}

void
free_pages(struct pg_alloc *pa, uval page_addr, uval bytes)
{
	lock_acquire(&pa->lock);
	debug("%s: %lx[%lx]\n", __func__, page_addr, bytes);
	__locked_set_page_bits(pa, page_addr, bytes, 1);
	lock_release(&pa->lock);
}

uval
get_pages_aligned(struct pg_alloc *pa, uval bytes, uval log_align)
{
	uval ret = PAGE_ALLOC_ERROR;
	uval num_pages= ALIGN_UP(bytes, 1 << pa->log_pgsize) >> pa->log_pgsize;

	lock_acquire(&pa->lock);

	uval loc = lbm_find_range(pa->lbm, 1, num_pages,
				  log_align - pa->log_pgsize,
				  pa->base);

	if (loc != LARGE_BITMAP_ERROR) {
		uval start_page = (loc + pa->base) << pa->log_pgsize;
		__locked_set_page_bits(pa, start_page, bytes, 0);
		ret = start_page;
	}
	debug("%s: %lx[%lx]\n", __func__, ret, bytes);
	lock_release(&pa->lock);
	return ret;
}

uval
get_pages(struct pg_alloc *pa, uval bytes)
{
	return get_pages_aligned(pa, bytes, pa->log_pgsize);
}

uval
get_zeroed_page(struct pg_alloc *pa)
{
	uval page = get_pages_aligned(pa, 1 <<  pa->log_pgsize,
				      pa->log_pgsize);

	assert(page != PAGE_ALLOC_ERROR, "ran out of pages\n");
	memset((void *)page, 0, 1<< pa->log_pgsize);

	return page;
}

