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

#include <config.h>
#include <types.h>
#include <hype.h>
#include <os.h>
#include <util.h>

#include <psm.h>
#include <objalloc.h>
#include <pgalloc.h>
#include <bitops.h>

static uval
psm_pte_hash(uval key, uval log_tbl_size)
{
	(void)log_tbl_size;
	return key >> LOG_PGSIZE;
}

static sval
psm_l_to_r(struct logical_chunk_info *lci, uval laddr, uval size)
{
	struct page_share_mgr *psm = PARENT_OBJ(typeof(*psm), psm_lci, lci);

	uval idx = (laddr - lci->lci_laddr) >> LOG_PGSIZE;

	uval raddr = INVALID_PHYSICAL_ADDRESS;

	lock_acquire(&psm->psm_lock);
	struct logical_chunk_info *next_lci = psm->psm_entries[idx];

	if (next_lci) {
		assert(next_lci->lci_raddr != INVALID_PHYSICAL_ADDRESS,
		       "PSM managed LCI's must use simple translation\n");
		assert(range_subset(next_lci->lci_laddr, next_lci->lci_size,
				    laddr, size), "range mismatch\n");

		raddr = laddr - next_lci->lci_laddr + next_lci->lci_raddr;
	}
	lock_release(&psm->psm_lock);
	return raddr;
}

static void
__psm_destroy(struct rcu_node *rcu)
{
	struct page_share_mgr *psm;
	psm = PARENT_OBJ(struct page_share_mgr, psm_lci.lci_rcu, rcu);

	hfree(psm, sizeof (*psm));
}

static sval
psm_destroy(struct logical_chunk_info *lci)
{
	rcu_defer(&lci->lci_rcu, __psm_destroy);
	return 0;
}

void
os_psm_destroy(struct page_share_mgr *psm)
{
	lock_acquire(&psm->psm_lock);
	__locked_psm_cleanup(psm);
	lock_release(&psm->psm_lock);
	rcu_defer(&psm->psm_lci.lci_rcu, __psm_destroy);
}

static sval
psm_unmap(struct logical_chunk_info *lci)
{
	uval idx = 0;
	struct page_share_mgr *psm = PARENT_OBJ(typeof(*psm), psm_lci, lci);

	lock_acquire(&psm->psm_lock);
	while (idx < NUM_PSM_ENTRIES) {
		lci = psm->psm_entries[idx];
		do {
			psm->psm_entries[idx++] = NULL;
		} while (psm->psm_entries[idx] == lci &&
			 idx < NUM_PSM_ENTRIES);
		__locked_psm_unmap_lci(psm, lci);
	}

	lock_release(&psm->psm_lock);
	return 0;
}

static sval
psm_export(struct logical_chunk_info *lci,
	   uval laddr, uval size, struct logical_chunk_info **plci)
{
	uval idx = (laddr - lci->lci_laddr) >> LOG_PGSIZE;

	assert(idx < NUM_PSM_ENTRIES, "bad laddr %lx\n", laddr);

	struct page_share_mgr *psm = PARENT_OBJ(typeof(*psm), psm_lci, lci);

	lock_acquire(&psm->psm_lock);
	struct logical_chunk_info *parent;

	parent = psm->psm_entries[idx];

	uval offset = laddr - parent->lci_laddr;

	if (offset + size > parent->lci_size) {
		lock_release(&psm->psm_lock);
		return H_Parameter;
	}

	parent = psm->psm_entries[idx];

	lock_release(&psm->psm_lock);

	return parent->lci_ops->export(parent, laddr, size, plci);
}

struct chunk_ops psm_ops = {
	.l_to_r = psm_l_to_r,
	.export = psm_export,
	.destroy = psm_destroy,
	.unmap = psm_unmap,
};

struct page_share_mgr *
init_os_psm(struct os *os)
{
	if (os->po_psm) {
		return os->po_psm;
	}

	struct page_share_mgr *psm = halloc(sizeof (*psm));
	struct lbm *x;

	psm->psm_entries = (struct logical_chunk_info **)
		get_pages(&phys_pa, NUM_PSM_ENTRIES * sizeof (void *));

	x = lbm_init(NUM_PSM_ENTRIES, sizeof (*x) + NUM_PSM_ENTRIES / 8,
		     &psm->psm_usage);
	assert(x, "lbm initialization failure\n");

	psm->psm_lci.lci_raddr = INVALID_PHYSICAL_ADDRESS;
	psm->psm_lci.lci_size = NUM_PSM_ENTRIES << LOG_PGSIZE;
	psm->psm_lci.lci_os = os;
	psm->psm_lci.lci_ops = &psm_ops;
	psm->psm_lci.lci_arch_ops = &psm_arch_ops;

	ht_init(&psm->psm_pte_ht, &psm_pte_hash);
	lock_init(&psm->psm_lock);
	lock_acquire(&psm->psm_lock);
	/* Try to install this object atomically */
	if (cas_uval((uval *)&os->po_psm, 0, (uval)psm)) {
		uval addr = set_lci(os, 0, &psm->psm_lci);

		assert(addr != INVALID_LOGICAL_ADDRESS, "Can't insert psm\n");
		lock_release(&psm->psm_lock);
		return os->po_psm;
	}

	/* Clean up, use the one already there */
	lock_release(&psm->psm_lock);
	free_pages(&phys_pa, (uval)psm->psm_entries, PGSIZE);
	hfree(psm, sizeof (*psm));

	return os->po_psm;
}

sval
psm_accept(struct page_share_mgr *psm, struct logical_chunk_info *lci)
{
	sval err = H_Resource;

	lock_acquire(&psm->psm_lock);

	/* We need to pick an alignment that is the greatest power of 2
	 * that divides size.  */
	uval pages = PGNUMS(lci->lci_size);
	uval idx = lbm_find_range(&psm->psm_usage, 0,
				  pages, bit_log2(pages & ~(pages - 1)), 0);

	if (idx == LARGE_BITMAP_ERROR) {
		goto unlock_error;
	}

	lbm_set_bits(&psm->psm_usage, idx, pages, 1);

	uval i = 0;

	while (i < pages) {
		psm->psm_entries[idx + i] = lci;
		++i;
	}

	uval base = (idx << LOG_PGSIZE) + psm->psm_lci.lci_laddr;

	lci->lci_laddr = base;
	lci->lci_os = psm->psm_lci.lci_os;
	err = H_Success;

/* *INDENT-OFF* */
unlock_error:
/* *INDENT-ON* */

	lock_release(&psm->psm_lock);
	return err;
}
