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
 * Page-share manager.
 *
 */
#ifndef _PAGE_SHARE_MANAGER_H
#define _PAGE_SHARE_MANAGER_H

#include <config.h>
#include <types.h>
#include <logical.h>
#include <dlist.h>
#include <hash.h>

#ifndef __ASSEMBLY__
extern struct arch_mem_ops psm_arch_ops;

enum {
	NUM_PSM_ENTRIES = (1UL << (4 + LOG_CHUNKSIZE - LOG_PGSIZE)),
};

struct page_share_mgr {
	struct logical_chunk_info psm_lci;
	lock_t psm_lock;
	struct logical_chunk_info **psm_entries;
	struct hash_table psm_pte_ht;
	struct lbm psm_usage;
	uval8 psm_bits[NUM_PSM_ENTRIES / 8];
};

static inline sval
psm_owns(struct page_share_mgr *psm, uval laddr)
{
	return range_contains(psm->psm_lci.lci_laddr, psm->psm_lci.lci_size,
			      laddr);
}

extern struct page_share_mgr *init_os_psm(struct os *os);

extern sval psm_accept(struct page_share_mgr *psm,
		       struct logical_chunk_info *lci);

extern void os_psm_destroy(struct page_share_mgr *psm);

extern void __locked_psm_cleanup(struct page_share_mgr *psm);

extern sval __locked_psm_unmap_lci(struct page_share_mgr *psm,
				   struct logical_chunk_info *lci);

static inline sval
psm_unmap_lci(struct page_share_mgr *psm, struct logical_chunk_info *lci)
{
	sval ret;

	lock_acquire(&psm->psm_lock);
	ret = __locked_psm_unmap_lci(psm, lci);
	lock_release(&psm->psm_lock);
	return ret;
}

#endif /* ! __ASSEMBLY__ */

#endif /* #ifndef _PAGE_SHARE_MANAGER_H */
