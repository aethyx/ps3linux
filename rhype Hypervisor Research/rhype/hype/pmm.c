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
 * $Id$
 *
 */
#include <config.h>
#include <lib.h>
#include <mmu.h>
#include <pmm.h>
#include <os.h>
#include <partition.h>
#include <hcall.h>
#include <logical.h>
#include <util.h>
#include <objalloc.h>
#include <pgalloc.h>
#include <hype.h>
#include <resource.h>
#include <debug.h>
#include <bitops.h>

void
logical_mmap_init(struct os *os)
{
	struct lbm *x;
	struct logical_memory_map *lmm = &os->logical_mmap;
	x = lbm_init(MAX_LOGICAL_MEM_CHUNKS,
		     sizeof (*x) + MAX_LOGICAL_MEM_CHUNKS / sizeof (uval),
		     &lmm->chunk_usage);

	assert(x, "lbm initialization failed\n");

	lmm->p_lci = (struct logical_chunk_info **)get_zeroed_page(&phys_pa);

	lock_init(&lmm->lmm_lock);
	dlist_init(&lmm->ps_list);
}

void
logical_mmap_clean(struct os *pos)
{
	uval i;

	for (i = 0; i < MAX_LOGICAL_MEM_CHUNKS; ++i) {
		assert(pos->logical_mmap.p_lci[i] == NULL,
		       "mem resources not detached\n");
	}
	free_pages(&phys_pa, (uval)pos->logical_mmap.p_lci, PGSIZE);
}

uval
logical_to_physical_address(struct os *os, uval laddr, uval len)
{
	struct logical_chunk_info *lci = laddr_to_lci(os, laddr);

	if (lci == NULL) {
		return INVALID_PHYSICAL_PAGE;
	}
	return lci_translate(lci, laddr, len);
}

uval
logical_to_physical_page(struct os *os, uval lpn)
{
	uval ret;
	uval laddr = lpn << LOG_PGSIZE;
	struct logical_chunk_info *lci = laddr_to_lci(os, laddr);

	if (lci == NULL) {
		return INVALID_PHYSICAL_PAGE;
	}
	ret = lci_translate(lci, laddr, PGSIZE);
	if (ret != INVALID_PHYSICAL_ADDRESS) {
		ret = ret >> LOG_PGSIZE;
	}
	return ret;
}

void
logical_fill_partition_info(struct os *os)
{
	int i;
	uval logical_chunk = 0;
	struct partition_info *pi = &os->cached_partition_info[1];

	if (os->os_partition_info == NULL ||
	    os->os_partition_info == os->cached_partition_info) {
		hputs("PMM: no partition_info structure in os image\n");
		return;
	}

	for (i = 0; i < MAX_MEM_RANGES; ++i) {
		pi->mem[i].addr = 0;
		pi->mem[i].size = -1;
	}

	for (i = 0; i < MAX_MEM_RANGES; ++i) {
		struct logical_chunk_info *lci;

		lci = os->logical_mmap.p_lci[logical_chunk];
		/* skip holes */
		while (!lci) {
			logical_chunk++;
			if (logical_chunk >= MAX_LOGICAL_MEM_CHUNKS)
				goto out;
			lci = os->logical_mmap.p_lci[logical_chunk];
		}

		pi->mem[i].addr = lci->lci_laddr;
		pi->mem[i].size = lci->lci_size;

		/* skip over all entries for this lci */
		while (lci == os->logical_mmap.p_lci[++logical_chunk] &&
		       logical_chunk < MAX_LOGICAL_MEM_CHUNKS) ;

		if (i > 1 &&
		    pi->mem[i - 1].addr + pi->mem[i - 1].size ==
		    pi->mem[i].addr) {
			pi->mem[i - 1].size += pi->mem[i].size;
			pi->mem[i].size = -1;
			pi->mem[i].addr = 0;
			--i;
		}
	}

/* *INDENT-OFF* */
out:
/* *INDENT-ON* */

	if (DEBUG_ACTIVE(DBG_MEMMGR)) {
		DEBUG_OUT(DBG_MEMMGR, "PMM: initialized mem info for LPID 0x%x\n",
				pi->lpid);
		for (i = 0; i < MAX_MEM_RANGES; ++i) {
			DEBUG_OUT(DBG_MEMMGR, "    start: 0x%lx (PA 0x%lx)  "
				  "size:0x%lx\n", pi->mem[i].addr,
				  logical_to_physical_address(os,
							      pi->mem[i].addr,
							      pi->mem[i].size),
				  pi->mem[i].size);
		}
	}

	copy_out(&os->os_partition_info[1], pi,
		 sizeof (os->cached_partition_info[1]));
}

struct logical_chunk_info *
laddr_to_lci(struct os *os, uval laddr)
{
	uval i = chunk_number_addr(laddr);
	struct logical_chunk_info *lci = NULL;

	lock_acquire(&os->logical_mmap.lmm_lock);

	lci = os->logical_mmap.p_lci[i];

	lock_release(&os->logical_mmap.lmm_lock);
	return lci;
}

/* If lci matches the lci already set for the hint_laddr, the lci
   entry is cleared */
uval
set_lci(struct os *os, uval hint_laddr, struct logical_chunk_info *lci)
{
	uval i = chunk_number_addr(hint_laddr);
	uval ret = INVALID_LOGICAL_ADDRESS;
	struct logical_memory_map *lmm = &os->logical_mmap;

	lock_acquire(&lmm->lmm_lock);

	uval size = ALIGN_UP(lci->lci_size + chunk_offset_addr(lci->lci_laddr),
			     CHUNK_SIZE);

	size >>= LOG_CHUNKSIZE;
	if (i + size > MAX_LOGICAL_MEM_CHUNKS) {
		i = 0;
	}

	uval log_align = bit_log2(size & ~(size - 1));

	if (!test_bits(&lmm->chunk_usage, i, size, 0)) {
		i = lbm_find_range(&lmm->chunk_usage, 0, size, log_align, 0);
	}

	if (i != LARGE_BITMAP_ERROR) {
		uval j = 0;

		lbm_set_bits(&lmm->chunk_usage, i, size, 1);
		for (; j < size; ++j) {
			lmm->p_lci[i + j] = lci;
		}
		ret = i << LOG_CHUNKSIZE;

		lci->lci_os = os;
		lci->lci_laddr = ret + chunk_offset_addr(lci->lci_laddr);
	}

	lock_release(&lmm->lmm_lock);
	return ret;

}

uval
detach_lci(struct logical_chunk_info *lci)
{
	uval ret = 1;
	uval idx = lci->lci_laddr >> LOG_CHUNKSIZE;
	struct logical_memory_map *lmm = &lci->lci_os->logical_mmap;

	if (idx < MAX_LOGICAL_MEM_CHUNKS && lmm->p_lci[idx] == lci) {
		lock_acquire(&lmm->lmm_lock);
		while (idx < MAX_LOGICAL_MEM_CHUNKS &&
		       lmm->p_lci[idx] && lmm->p_lci[idx] == lci) {
			lmm->p_lci[idx] = NULL;
			ret = 0;
			lbm_set_bits(&lmm->chunk_usage, idx, 1, 0);
			++idx;
		}

		lci_disable(lci);
		lock_release(&lmm->lmm_lock);
	}
	return ret;
}
