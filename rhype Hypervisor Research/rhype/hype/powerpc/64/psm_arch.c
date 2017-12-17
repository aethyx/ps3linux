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
 */

#include <config.h>
#include <types.h>
#include <hype.h>
#include <os.h>

#include <psm.h>
#include <objalloc.h>
#include <pgalloc.h>

struct pte_hash_ent {
	struct hash_entry phe_he;
	union pte *phe_pte;
};

static uval
psm_sav_ptep(struct logical_chunk_info *lci, uval laddr, uval rpn,
	     union pte *pte)
{
	(void)rpn;

	struct page_share_mgr *psm = PARENT_OBJ(typeof(*psm), psm_lci, lci);

	lock_acquire(&psm->psm_lock);
	struct pte_hash_ent *phe = halloc(sizeof (*phe));

	phe->phe_pte = pte;
	phe->phe_he.he_key = laddr;
	ht_insert(&psm->psm_pte_ht, &phe->phe_he);

	uval idx = (laddr - psm->psm_lci.lci_laddr) >> LOG_PGSIZE;
	struct logical_chunk_info *real_lci = psm->psm_entries[idx];

	atomic_add32(&real_lci->lci_ptecount, 1);

	lock_release(&psm->psm_lock);

	return 1;
}

static uval
psm_rem_ptep(struct logical_chunk_info *lci, uval laddr, uval rpn,
	     union pte *pte)
{
	(void)rpn;
	struct page_share_mgr *psm = PARENT_OBJ(typeof(*psm), psm_lci, lci);

	lock_acquire(&psm->psm_lock);

	/* We have to deconstruct the hash because we need to match a
	 * non-hash-key item */
	struct hash_table *ht = &psm->psm_pte_ht;

	uval hash = ht->ht_hash(laddr, ht->ht_log_tbl_size);

	hash &= (1UL << ht->ht_log_tbl_size) - 1;

	struct hash_entry *he = ht->ht_table[hash];
	struct hash_entry **prev = &ht->ht_table[hash];

	while (he != NULL) {
		struct pte_hash_ent *phe =
			PARENT_OBJ(typeof(*phe), phe_he, he);
		if (he->he_key == laddr && phe->phe_pte == pte) {
			*prev = he->he_next;
			hfree(phe, sizeof (*phe));
			break;
		}

		prev = &he->he_next;
		he = he->he_next;
	}

	uval idx = (laddr - psm->psm_lci.lci_laddr) >> LOG_PGSIZE;
	struct logical_chunk_info *real_lci = psm->psm_entries[idx];

	atomic_add32(&real_lci->lci_ptecount, ~0);

	lock_release(&psm->psm_lock);
	return 1;
}

static uval
psm_check_WIMG(struct logical_chunk_info *lci, uval laddr, union ptel pte)
{
	struct page_share_mgr *psm = PARENT_OBJ(typeof(*psm), psm_lci, lci);

	uval idx = chunk_offset_page(pte.bits.rpn) >> LOG_PGSIZE;

	uval ret = 0;

	lock_acquire(&psm->psm_lock);
	struct logical_chunk_info *next_lci = psm->psm_entries[idx];

	if (next_lci) {
		ret = next_lci->lci_arch_ops->amo_check_WIMG(next_lci,
							     laddr, pte);
	}

	lock_release(&psm->psm_lock);
	return ret;
}

struct arch_mem_ops psm_arch_ops = {
	.amo_sav_ptep = &psm_sav_ptep,
	.amo_rem_ptep = &psm_rem_ptep,
	.amo_check_WIMG = &psm_check_WIMG,
};

void
__locked_psm_cleanup(struct page_share_mgr *psm)
{
	struct hash_entry *he;

	while ((he = ht_pop(&psm->psm_pte_ht))) {
		struct pte_hash_ent *phe;

		phe = PARENT_OBJ(typeof(*phe), phe_he, he);
		hfree(phe, sizeof (*phe));

	}

}

sval
__locked_psm_unmap_lci(struct page_share_mgr *psm,
		       struct logical_chunk_info *lci)
{
	assert(lci->lci_flags & LCI_DISABLED, "expecting disabled LCI\n");
	uval idx = (lci->lci_laddr - psm->psm_lci.lci_laddr) >> LOG_PGSIZE;

	struct hash_entry *he;

	/* Check hash table for any entires in our range */
	uval start = lci->lci_laddr;

	if (lci->lci_ptecount == 0) {
		return 0;
	}

	while (start < lci->lci_laddr + lci->lci_size) {
		psm->psm_entries[idx++] = NULL;
		while ((he = ht_remove(&psm->psm_pte_ht, start))) {

			struct pte_hash_ent *phe;

			phe = PARENT_OBJ(typeof(*phe), phe_he, he);
			pte_invalidate(&lci->lci_os->htab, phe->phe_pte);
			atomic_add32(&lci->lci_ptecount, ~0);

			hfree(phe, sizeof (*phe));

			if (lci->lci_ptecount == 0) {
				return 0;
			}
		}
		start += PGSIZE;
	}

	return 0;
}
