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
 * A simple object allocator.
 */
#include <config.h>
#include <lib.h>
#include <mmu.h>
#include <dlist.h>
#include <util.h>
#include <pgalloc.h>
#include <objalloc.h>
#include <bitops.h>

#undef  DEBUG_ALLOCATOR
#ifdef DEBUG_ALLOCATOR
#define debug(ARGS...) hprintf(ARGS)
#else
#define debug(ARGS...)
#endif

enum {
	LOG_MIN_OBJ_SIZE	=5,
	LOG_MAX_OBJ_SIZE	=(LOG_PGSIZE - 1),
	NUM_CACHES		=(LOG_MAX_OBJ_SIZE - LOG_MIN_OBJ_SIZE + 1)
};

static inline uval
set_zero_bit(uval32 *set, uval numbits)
{
	uval i = 0;
	int idx = 0;

	while (i <= numbits / 32) {
		idx = ffz(set[i]);
		if (idx < 32) {
			break;
		}
		++i;
	}
	if (idx + (32 * i) >= numbits)
		return numbits;

	set[i] |= 1ULL << idx;
	return idx + (32 * i);
}


struct alloc_page_desc {
	struct alloc_page_desc *next;
	char*	page;
#define APD_BITS	(PGSIZE / (1UL << LOG_MIN_OBJ_SIZE))
#define APD_WORDS	((APD_BITS + 31) / 32)
	uval32	bits[APD_WORDS];
};

struct free_obj {
	struct dlist list;
	struct alloc_page_desc *node;
};

struct objcache {
	struct alloc_page_desc *node;
	struct pg_alloc *pa;
	uval	size;
	struct free_obj *list;
	lock_t	lock;
	uval	alloced;
};


#define HDR_PAGE_COUNT ((PGSIZE/sizeof(struct objcache)))
#define USED_SIZE	((HDR_PAGE_COUNT+31)/32)
struct page_desc_cache {
	union {
		struct {
			struct page_desc_cache *next;
			uval32	used[USED_SIZE];
		} hdr;
		struct alloc_page_desc node[HDR_PAGE_COUNT];
	};
};
lock_t pdc_lock;

struct objcache head_cache;

static struct page_desc_cache* nodes = NULL;
static int free_nodes = 0;

static struct alloc_page_desc *
new_alloc_page_desc(struct pg_alloc *pa)
{
	struct page_desc_cache *pdc;
	int idx = -1;

	lock_acquire(&pdc_lock);

	if (free_nodes == 0) {
		uval x;
		pdc = (typeof(pdc))get_pages(pa, PGSIZE);
		debug("Allocator page: %p\n", pdc);

		memset(pdc, 0, sizeof(struct page_desc_cache));
		pdc->hdr.next = nodes;

		// Start of the page consists of the pdc struct,
		// thus we need to mark some alloc_page_descs as used
		x = (sizeof(pdc->hdr) + sizeof(pdc->node[0]))/
			sizeof(pdc->node[0]);

		pdc->hdr.used[0] = (1<<x)-1;

		nodes = pdc;
		free_nodes += HDR_PAGE_COUNT - x;
	}

	pdc = nodes;
	while (pdc) {
		idx = set_zero_bit(pdc->hdr.used, HDR_PAGE_COUNT);
		if (idx < (int)HDR_PAGE_COUNT) break;
		pdc = pdc->hdr.next;
	}
	assert(pdc, "out of alloc_page_descs");

	--free_nodes;


	lock_release(&pdc_lock);
	if (idx == -1) {
		return (struct alloc_page_desc*)NULL;
	}
	return &pdc->node[idx];

}

void*
cache_alloc(struct objcache* hdr)
{

	int idx;
	struct free_obj *obj;

	lock_acquire(&hdr->lock);

	if (!hdr->list) {
		struct alloc_page_desc *apd = new_alloc_page_desc(hdr->pa);
		uval i = 0;
		uval num_objs = PGSIZE/hdr->size;
		apd->page = (char*)get_pages(hdr->pa, PGSIZE);
		for (; i<num_objs; ++i) {
			struct free_obj* fo =
				(typeof(fo))(apd->page + i*hdr->size);
			dlist_init(&fo->list);
			fo->node = apd;
			if (!hdr->list) {
				hdr->list = fo;
			} else {
				dlist_insert(&fo->list, &hdr->list->list);
			}
		}
		apd->next = hdr->node;
		hdr->node = apd;
	}

	obj = hdr->list;
	if (dlist_empty(&hdr->list->list)) {
		hdr->list = NULL;
	} else {
		hdr->list = (typeof(hdr->list))obj->list.dl_next;
		dlist_detach(&obj->list);
	}
	/* Mark the appropriate bit as used in the node object */
	idx = (((uval)obj) & (PGSIZE-1))/hdr->size;
	if (obj->node) {
		obj->node->bits[idx/32] |= 1<<(idx%32);

		assert((((uval)obj) & ~(PGSIZE-1)) == ((uval)obj->node->page),
		       "Object not on right page for node\n");
	}

	memset(obj, 0, hdr->size);

	hdr->alloced += hdr->size;
	lock_release(&hdr->lock);

	return obj;
}

void
cache_free(struct objcache* cache, void* ptr)
{
	struct free_obj* fo = (typeof(fo))ptr;
	char* page = (char*)ALIGN_DOWN(((uval)ptr), (uval)PGSIZE);

	lock_acquire(&cache->lock);

	dlist_init(&fo->list);
	if (page == cache->node->page) {
		int objnum = (((char*)ptr)-page)/cache->size;
		fo->node = cache->node;
		fo->node->bits[objnum/32] &= ~(1<<(objnum%32));
	} else {
		fo->node = NULL;
	}
	if (cache->list) {
		dlist_insert(&fo->list, &cache->list->list);
	}
	cache->list = fo;

	assert(cache->alloced != (uval)0, "too many free's\n");
	cache->alloced -= cache->size;
	lock_release(&cache->lock);
}


void
cache_assert(struct objcache *cache)
{
	struct free_obj *obj = cache->list;
	uval count = 0;
	uval bitcount = 0;
	uval pgcount = 0;
	if (obj) {
		do {
			++count;
			obj = (struct free_obj *) dlist_next(&obj->list);
		} while (obj != cache->list);
	}

	struct alloc_page_desc *node = cache->node;

	while (node) {
		assert(node->page, "node without page\n");
		node = node->next;
		++pgcount;

		uval32 idx = 0;
		uval32 bit;
		uval numbits = PGSIZE / cache->size;
		while (numbits) {
			bit = node->bits[idx++];
			numbits -= 32;

			while (bit) {
				if (bit & 1) {
					++bitcount;
				}
				bit = bit >> 1;
			}
		}
	}

	assert((count + cache->alloced/cache->size) ==
	       (pgcount * PGSIZE / cache->size),
	       "Alloc cache corruption\n");

	assert(bitcount == count, "bitcount mismatch\n");
	hprintf("Cache: %p/%ld  used: %lx/%lx pages: %lx\n",
		cache, cache->size, cache->alloced, count * cache->size,
		pgcount);
}

static struct objcache caches[NUM_CACHES];

void
halloc_assert(void)
{
	uval i = 0;
	while (i < NUM_CACHES) {
		cache_assert(&caches[i]);
		++i;
	}
}

void
cache_init(struct pg_alloc *pa, struct objcache* cache, uval size)
{
	cache->pa   = pa;
	cache->size = size;
	cache->node = NULL;
	cache->list = NULL;
	lock_init(&cache->lock);
}


static struct pg_alloc *default_pa = NULL;
void
init_allocator(struct pg_alloc *pa)
{
	int i = 0;
	lock_init(&pdc_lock);
	default_pa = pa;
	for (; i<NUM_CACHES; ++i) {
		cache_init(pa, &caches[i],1<<(i+LOG_MIN_OBJ_SIZE));
	}
}


void*
halloc(uval size)
{
	uval i = 0;

	if (size >= (1<<LOG_MAX_OBJ_SIZE)) {
		void *ptr = (void*)get_pages(default_pa, size);
		debug("alloc: %p %lx\n", ptr, size);
		return ptr;
	}

	assert(size <= (1<<LOG_MAX_OBJ_SIZE),
	       "requested size too big\n");
	for (; i<NUM_CACHES; ++i) {
		if (size <= (1ULL<<(i+LOG_MIN_OBJ_SIZE))) break;
	}

	void *ptr = cache_alloc(&caches[i]);
	debug("alloc: %p %lx\n", ptr, size);
	return ptr;
}

void
hfree(void* ptr, uval size)
{
	uval i = 0;
	assert(ptr, "freeing NULL\n");
	debug("free:  %p %lx\n", ptr, size);
	if (size >= (1<<LOG_MAX_OBJ_SIZE)) {
		size = ALIGN_UP(size, PGSIZE);

		if (size > 64 * PGSIZE) {
			size = ALIGN_UP(size, 64 * PGSIZE);
		}

		free_pages(default_pa, (uval)ptr, size);
		return;
	}

	assert(size <= (1<<LOG_MAX_OBJ_SIZE),
	       "requested size too big\n");
	for (; i<NUM_CACHES; ++i) {
		if (size <= (1ULL<<(i+LOG_MIN_OBJ_SIZE))) break;
	}

	cache_free(&caches[i],ptr);

}

// WARNING: untested code
// Until we get a free_page interface, this is useless
static uval
release_apd(struct objcache* cache, struct alloc_page_desc* apd)
{
	struct free_obj* curr;
	uval numobjs = PGSIZE / cache->size;
	uval objnum;
	struct page_desc_cache *pdc;
	uval i;

	for (i = 0; (i < APD_WORDS) && (apd->bits[i]==0); ++i);
	if (i < APD_WORDS)
		return 0;

	for (i = 0; i < numobjs; ++i) {
		// All bits set to 0 --> all objects free
		curr = (struct free_obj*)apd->page + numobjs * i;

		if (cache->list == curr) {
			cache->list= (struct free_obj*)dlist_next(&curr->list);
			if (cache->list == curr) {
				cache->list = NULL;
			}
		}
		dlist_detach(&curr->list);
	}

	free_pages(cache->pa, (uval)apd->page, PGSIZE);

	lock_acquire(&pdc_lock);

	// Locate the page_desc_cache and clear the bit for this apd
	objnum = (((uval)apd) - ALIGN_DOWN((uval)apd,(uval)PGSIZE)) /
		sizeof(struct alloc_page_desc);

	pdc = (struct page_desc_cache*)ALIGN_DOWN((uval)apd, (uval)PGSIZE);

	pdc->hdr.used[objnum/32] &= ~(1<<(objnum%32));
	++free_nodes;

	lock_release(&pdc_lock);

	return 1;
}

void
cache_clean(struct objcache * cache)
{
	struct free_obj* curr;
	struct free_obj* next;
	struct alloc_page_desc* next_apd;
	struct alloc_page_desc* apd;
	struct alloc_page_desc** prev;
	assert(cache, "NULL cache pointer\n");
	if (!cache->list)
		return;

	lock_acquire(&cache->lock);

	apd = cache->node;
	prev = &cache->node;

	while (apd) {
		curr = cache->list;
		do {
			char* page = (char*)ALIGN_DOWN(((uval)curr),
						       (uval)PGSIZE);
			int objnum = (((char*)curr)-page)/cache->size;
			next = (struct free_obj*)dlist_next(&curr->list);

			if (page != apd->page) {
				continue;
			}
			curr->node = apd;
			curr->node->bits[objnum/32] &= ~(1<<(objnum%32));
		} while ((curr = next) != cache->list);

		//Try to release the apd object
		next_apd = apd->next;
		if (release_apd(cache, apd)) {
			*prev = next_apd;
			apd = next_apd;

		} else {
			apd = apd->next;
		}
	}

	lock_release(&cache->lock);
}
