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
 * Implement the page table cache.
 */

#include <config.h>
#include <hype.h>
#include <lib.h>
#include <pmm.h>
#include <mmu.h>
#include <vm.h>
#include <timer.h>
#include <pgcache.h>
#include <pgalloc.h>

#define	NR_MMU_PDES	1024
#define	NR_MMU_PTES	1024
#define	NR_HV_PGTABS	(HV_VSIZE / (1 << LOG_PDSIZE))
#define	NR_LPAR_PGTABS	(NR_MMU_PDES - NR_HV_PGTABS)

/*
 * Initialize page cache for a given partition
 */
void
pgc_init(struct os *os)
{
	struct pgcache *pgc = &os->pgcache;
	uval i;

	/*
	 * XXX FIXME! Instead of a static allocation, the page cache
	 * size should be proportional to the amount of physical memory
	 * assigned to this partition.
	 */

	assert(NR_DIR_FRAMES >= 3, "too few directory frames");
	assert(sizeof (union pgframe) == 32, "bad pgframe size");

	/* allocate page frames */
	pgc->frames = (union pgframe *)
		get_pages(&phys_pa, NR_FRAMES * sizeof (union pgframe));

	/* initialize directory frames */
	pgc->freelist[PGC_PGDIR] = pgc->frames;
	pgc->freecount[PGC_PGDIR] = NR_DIR_FRAMES;
	for (i = 0; i < NR_DIR_FRAMES; i++) {
		pgc->frames[i].pgdir.lp_vaddr =
			(uval *)get_zeroed_page(&phys_pa);

		pgc->frames[i].pgdir.hv_vaddr =
			(uval *)get_zeroed_page(&phys_pa);

		pgc->frames[i].pgdir.hv_paddr =
			physical((uval)pgc->frames[i].pgdir.hv_vaddr);
		assert(pgc->frames[i].pgdir.hv_paddr !=
		       INVALID_VIRTUAL_ADDRESS, "bad vaddr\n");

		pgc->frames[i].pgdir.pgt =
			(union pgframe **)get_zeroed_page(&phys_pa);

		pgc->frames[i].pgdir.next = &pgc->frames[i + 1];
	}
	pgc->frames[NR_DIR_FRAMES - 1].pgdir.next = NULL;

	/* initializee page table frames */
	pgc->freelist[PGC_PGTAB] = &pgc->frames[NR_DIR_FRAMES];
	pgc->freecount[PGC_PGTAB] = NR_FRAMES - NR_DIR_FRAMES;
	for (i = NR_DIR_FRAMES; i < NR_FRAMES; i++) {
		pgc->frames[i].pgtab.lp_vaddr =
			(uval *)get_zeroed_page(&phys_pa);

		pgc->frames[i].pgtab.hv_vaddr =
			(uval *)get_zeroed_page(&phys_pa);

		pgc->frames[i].pgtab.hv_paddr =
			physical((uval)pgc->frames[i].pgtab.hv_vaddr);
		assert(pgc->frames[i].pgtab.hv_paddr !=
		       INVALID_VIRTUAL_ADDRESS, "bad vaddr\n");

		pgc->frames[i].pgtab.next = &pgc->frames[i + 1];
	}
	pgc->frames[NR_FRAMES - 1].pgdir.next = NULL;
}

/*
 * Free pages used for page cache for a given partition
 *
 * Note the use of hv_unmap for freeing all pages.  This is necessitated
 * by pgc_alloc remapping some pages.  While not all pages need to be
 * unmapped, since unmap just restores the original mapping, this does
 * not hurt, except for more cycles consumed.
 */
void
pgc_destroy(struct os *os)
{
	struct pgcache *pgc = &os->pgcache;
	uval i;

	for (i = 0; i < NR_DIR_FRAMES; i++) {
		hv_unmap((uval)pgc->frames[i].pgdir.lp_vaddr, PGSIZE);
		hv_unmap((uval)pgc->frames[i].pgdir.hv_vaddr, PGSIZE);
		hv_unmap((uval)pgc->frames[i].pgdir.pgt, PGSIZE);
	}
	for (i = NR_DIR_FRAMES; i < NR_FRAMES; i++) {
		hv_unmap((uval)pgc->frames[i].pgtab.lp_vaddr, PGSIZE);
		hv_unmap((uval)pgc->frames[i].pgtab.hv_vaddr, PGSIZE);
	}
	free_pages(&phys_pa, (uval)pgc->frames,
		   NR_FRAMES * sizeof (union pgframe));
}

/*
 * Map hypervisor into page directory
 */
void
pgc_hypervisor_mapping(union pgframe *pgd)
{
	uval pdi;

	/* map in the hypervisor pages */
	for (pdi = NR_LPAR_PGTABS; pdi < NR_MMU_PTES; pdi++)
		pgd->pgdir.hv_vaddr[pdi] = hv_pgd[pdi];
}

/*
 * Create an initial mapping for a partition.
 * That is, map in the hypervisor and set up a sequential
 * mapping for the first CHUNK_SIZE of partition memory.
 */
void
pgc_initial_mapping(struct cpu_thread *thread)
{
	union pgframe *pgd = thread->pgd;
	uval pdi;
	uval laddr;

	/* map in the hypervisor pages */
	pgc_hypervisor_mapping(pgd);

	/* create a 1-1 mapping for the first chunk */
	for (laddr = 0, pdi = 0; laddr < CHUNK_SIZE; laddr += LPGSIZE, pdi++) {
		int rc = pgc_set_pte(thread, pgd, pdi, laddr | PTE_PS |
				     PTE_US | PTE_RW | PTE_A | PTE_D | PTE_P);
		assert(rc, "unable to create 1-1 mapping");
	}
}

/*
 * Enqueue/dequeue. The freelist is managed through these functions.
 */
static inline void
pgc_enqueue(struct pgcache *pgc, int type, union pgframe *frame)
{
	assert(frame != NULL, "invalid frame");
	frame->cmn.next = pgc->freelist[type];
	pgc->freelist[type] = frame;
	pgc->freecount[type]++;
}

static inline union pgframe *
pgc_dequeue(struct pgcache *pgc, int type)
{
	union pgframe *frame = pgc->freelist[type];

	if (frame == NULL)
		return NULL;
	pgc->freelist[type] = frame->cmn.next;
	pgc->freecount[type]--;
	return frame;
}

/*
 * Hash table management. Frames that need to be looked up quickly
 * are stored in a hash table.
 */
static void
pgc_enter(struct pgcache *pgc, union pgframe *frame, int type)
{
	int hash = HASH(frame->cmn.lp_laddr);

	assert(type >= PGC_PGDIR && type < PGC_NTYPES - 1, "invalid type");

	frame->cmn.next = pgc->htab[type][hash];
	pgc->htab[type][hash] = frame;
}

static void
pgc_remove(struct pgcache *pgc, union pgframe *frame, int type)
{
	int hash = HASH(frame->cmn.lp_laddr);

	assert(type >= PGC_PGDIR && type < PGC_NTYPES - 1, "invalid type");

	union pgframe *h = pgc->htab[type][hash];

	if (h == frame) {
		pgc->htab[type][hash] = h->cmn.next;
	} else {
		for (; h->cmn.next; h = h->cmn.next) {
			if (h->cmn.next == frame) {
				h->cmn.next = frame->cmn.next;
				break;
			}
		}
	}
}

/*
 * Garbage collect until a frame of given type becomes
 * available. This routine cannot fail. In the worst
 * case we free a frame that is currently in use.
 */
static void
pgc_garbage_collect(struct cpu_thread *thread, int type)
{
	struct pgcache *pgc = &thread->cpu->os->pgcache;
	union pgframe *victim;
	union pgframe *list;
	uval h;
	uval index;

	assert(pgc->freecount[type] == 0, "free frames available");

	/*
	 * Before we start freeing resources that are potentially
	 * still in use, we free any resources that the partition
	 * identified as "flushed".
	 */
	if (thread->flushed && thread->flushed != thread->pgd) {
		pgc_free(pgc, thread->flushed, PGC_PGDIR);
		thread->flushed = NULL;
		if (pgc->freelist[type]) {
			return;
		}
	}
#ifdef PGC_DEBUG
	hprintf("pgc_garbage_collect(type %d)\n", type);
#endif

	/*
	 * 1st attempt: Find the page directory that hasn't been used
	 * recently and free it. This will result in one free page
	 * directory frame and hopefully many page table frames. If
	 * we don't get the desired frame, we keep freeing page
	 * directories until we get the desired frame, or hit the
	 * current page directory.
	 */
	do {
		for (victim = NULL, h = 0; h < HASHSIZE; h++) {
			list = pgc->htab[PGC_PGDIR][h];
			for (; list; list = list->cmn.next) {
				if (victim == NULL) {
					victim = list;
				} else if (victim->pgdir.last >=
					   list->pgdir.last) {
					victim = list;
				}
			}
		}

		/* we should always find ourselves */
		assert(victim != NULL, "no potential victim");
		if (victim != thread->pgd)
			pgc_free(pgc, victim, PGC_PGDIR);

		/* did this free a resource we needed? */
		if (pgc->freelist[type] != NULL)
			return;
	} while (victim != NULL && victim != thread->pgd);

	/*
	 * 2nd attempt: At this point we should only be looking for
	 * a page table frames. Apparently, all the page tables
	 * are in use by the current page directory (this indicates a
	 * serious under commitment of resources problem).
	 * At this point there is not much we can do. We randomly
	 * select a page table frame from the current active set and
	 * free it.
	 */
	assert(type == PGC_PGTAB, "expected PGTAB");

	/* random enough for our purpose */
	index = ticks % (NR_FRAMES - NR_DIR_FRAMES);
	assert(pgc->frames[NR_DIR_FRAMES + index].pgtab.pgd != NULL, "no pgd");
	pgc_free(pgc, &pgc->frames[NR_DIR_FRAMES + index], PGC_PGTAB);

	assert(pgc->freelist[type] != NULL, "unable to free resource");

	hprintf("Warning: Freeing active page table from partiton 0x%x.\n",
		thread->cpu->os->po_lpid);
}

/*
 * Allocate a page frame of given type.
 */
union pgframe *
pgc_alloc(struct cpu_thread *thread, uval laddr, int type)
{
	struct pgcache *pgc = &thread->cpu->os->pgcache;
	union pgframe *frame;
	uval paddr = INVALID_PHYSICAL_ADDRESS;

	if (laddr != INVALID_PHYSICAL_ADDRESS) {
		paddr = logical_to_physical_address(thread->cpu->os,
						    laddr, PGSIZE);
		if (paddr == INVALID_PHYSICAL_ADDRESS)
			return NULL;
	}
#ifdef PGC_DEBUG
	hprintf("pgc_alloc(lpid %x, laddr 0x%lx, type %d), free %ld\n",
		thread->cpu->os->po_lpid, laddr, type, pgc->freecount[type]);
#endif

	/* dequeue from the free list */
	if (pgc->freelist[type] == NULL)
		pgc_garbage_collect(thread, type);

	frame = pgc_dequeue(pgc, type);
	assert(frame != NULL, "Empty free list");	/* cannot happen */

	/* setup the mapping */
	frame->cmn.lp_laddr = laddr;
	if (paddr != INVALID_PHYSICAL_ADDRESS) {
		hv_map((uval)frame->cmn.lp_vaddr, paddr, PGSIZE, PTE_RW, 1);
	}
	memset(frame->cmn.hv_vaddr, 0, PGSIZE);
	/* cmn.hv_paddr is already initialized */

	if (type == PGC_PGDIR) {
		frame->pgdir.last = ticks;
		pgc_enter(pgc, frame, type);	/* keep it in the hash table */
	}

	return frame;
}

/*
 * Free a page table, including the link in the page table
 * directory to the page table. Make sure we zap the entry
 * so that no information is leaked.
 */
static void
pgc_free_pgtab(struct pgcache *pgc, union pgframe *pgtab)
{
	union pgframe *pgd = pgtab->pgtab.pgd;

#ifdef PGC_DEBUG
	hprintf("pgc_free_pgtab: laddr 0x%lx (pgd %p, pdi %ld)\n",
		pgtab->pgtab.lp_laddr, pgd, pgd->pgtab.pdi);
#endif

	assert(pgd != NULL, "null pgd");
	assert(pgtab->pgtab.pdi < NR_MMU_PTES, "invalid pdi");

	/* zap entry */
	pgtab->pgtab.pgd = NULL;
	memset(pgtab->pgtab.hv_vaddr, 0, PGSIZE);

	/* zap backlink */
	pgd->pgdir.pgt[pgtab->pgtab.pdi] = NULL;

	/* zero PDE */
	pgd->pgdir.hv_vaddr[pgtab->pgtab.pdi] = 0;

	/* insert it into the freelist */
	pgc_enqueue(pgc, PGC_PGTAB, pgtab);
}

/*
 * Free a page table directory and free any page tables
 * associated with it. Make sure the entries are zapped
 * so that no information is leaked.
 */
static void
pgc_free_pgdir(struct pgcache *pgc, union pgframe *pgdir)
{
	union pgframe *pgtab;
	uval pdi;

	for (pdi = 0; pdi < NR_LPAR_PGTABS; pdi++)
		if ((pgtab = pgdir->pgdir.pgt[pdi]) != NULL)
			pgc_free_pgtab(pgc, pgtab);

	/* zap entry */
	/* pgdir->pgdir.pgt is already zeroed */
	memset(pgdir->pgdir.hv_vaddr, 0, PGSIZE);

	/* remove it from the hash table */
	pgc_remove(pgc, pgdir, PGC_PGDIR);

	/* insert it into the freelist */
	pgc_enqueue(pgc, PGC_PGDIR, pgdir);
}

/*
 * Free a frame of given type
 */
void
pgc_free(struct pgcache *pgc, union pgframe *frame, int type)
{
	switch (type) {
	case PGC_PGTAB:
		pgc_free_pgtab(pgc, frame);
		break;

	case PGC_PGDIR:
		pgc_free_pgdir(pgc, frame);
		break;
	}
}

/*
 * Flush all the shadow page tables
 */
void
pgc_flush(struct cpu_thread *thread, union pgframe *pgd)
{
	struct pgcache *pgc = &thread->cpu->os->pgcache;
	uval pdi;

	/* iterate over all the guest pages */
	for (pdi = 0; pdi < NR_LPAR_PGTABS; pdi++) {
		uval32 pde = pgd->pgdir.hv_vaddr[pdi];

		if ((pde & PTE_P) == 0)
			continue;

		if ((pde & PTE_PS) == 0)
			pgc_free(pgc, pgd->pgdir.pgt[pdi], PGC_PGTAB);
		else
			pgc_set_pte(thread, pgd, pdi, 0);
	}
}

/*
 * Prefetch a page table
 */
static uval
pgc_prefetch_pgtab(struct cpu_thread *thread, union pgframe *table)
{
	uval i;
	uval status = 1;

	for (i = 0; i < NR_MMU_PTES; i++)
		if (!pgc_set_pte(thread, table, i, table->pgtab.lp_vaddr[i]))
			status = 0;
	return status;
}

/*
 * Prefetch a whole page directory. This requires some care. When this
 * routine finishes the shadow copy and the guest page tables are guaranteed
 * to be consistent. This guarantee obviates the need for the guest
 * to H_Page_Dir_Flush on a h_page_dir invocation, this is implicit.
 * If the guest page tables are invalid, their corresponding shadow
 * copy entries will be not-present null entries.
 */
static uval
pgc_prefetch_pgdir(struct cpu_thread *thread, union pgframe *pgd)
{
	struct pgcache *pgc = &thread->cpu->os->pgcache;
	uval pdi;
	uval status = 1;

	/* iterate over all the guest pages */
	for (pdi = 0; pdi < NR_LPAR_PGTABS; pdi++) {
		uval32 pde = pgd->pgdir.lp_vaddr[pdi];
		uval32 flags = pte_flags(pde);
		uval32 laddr = pte_pageaddr(pde);
		union pgframe *pgt = pgd->pgdir.pgt[pdi];

		/* pgtab/page is absent */
		if ((flags & PTE_P) == 0) {
			if (pgd->pgdir.pgt[pdi])
				pgc_free(pgc, pgt, PGC_PGTAB);
			pgc_set_pte(thread, pgd, pdi, 0);
			continue;
		}

		/* handle large pages */
		if (flags & PTE_PS) {
			if (pgd->pgdir.pgt[pdi])
				pgc_free(pgc, pgd->pgdir.pgt[pdi], PGC_PGTAB);
			pgc_set_pte(thread, pgd, pdi, pde);
			continue;
		}

		/* existing pgtab */
		if (pgt != NULL) {
			if (pgt->pgtab.lp_laddr == laddr) {
				pgc_prefetch_pgtab(thread, pgt);
				continue;
			}
			/* pgtab changed */
			pgc_free(pgc, pgt, PGC_PGTAB);
		}

		/* new pgtab */
		pgt = pgc_alloc(thread, laddr, PGC_PGTAB);
		if (pgt != NULL) {
			pgc_set_pgtab(pgd, pdi, pgt, flags);
			pgc_prefetch_pgtab(thread, pgt);
		} else {
			status = 0;	/* error */
			pgc_set_pte(thread, pgd, pdi, 0);
		}
	}

	return status;
}

/*
 * Prefetch the shadow page directories/tables from the tables kept in the
 * partition. Fetching the shadow page table implies a flush.
 */
uval
pgc_prefetch(struct cpu_thread *thread, union pgframe *table, int type)
{
	switch (type) {
	case PGC_PGTAB:
		return pgc_prefetch_pgtab(thread, table);

	case PGC_PGDIR:
		return pgc_prefetch_pgdir(thread, table);
	}
	return 0;
}

#ifdef DEBUG
/*
 * Dump page table in some compact format
 */
static void
dump_pte(uval vaddr, uval *tab, uval first, uval last, int large)
{
	uval size;
	uval base;
	uval flags = pte_flags(tab[first]);

	size = large ? LPGSIZE : (last - first) * PGSIZE;
	base = pte_pageaddr(tab[first]);

	hprintf("0x%08lx..0x%08lx - ", vaddr, vaddr + size - 1);
	hprintf("0x%08lx..0x%08lx ", base, base + size - 1);

	if (!large)
		hprintf("[%ld]", last - first);
	if (flags & PTE_P)
		hprintf(" P");
	if (flags & PTE_RW)
		hprintf(" RW");
	if (flags & PTE_US)
		hprintf(" US");
	if (flags & PTE_PWT)
		hprintf(" PWT");
	if (flags & PTE_PCD)
		hprintf(" PCD");
	if (flags & PTE_A)
		hprintf(" A");
	if (flags & PTE_D)
		hprintf(" D");
	if (large && (flags & PTE_PS))
		hprintf(" PS");
	if (!large && flags & PTE_PTAI)
		hprintf(" PTAI");
	hprintf(" (0x%lx)\n", (flags & PTE_SM) >> 9);
}

static int
same_flags(uval *tab, uval first, uval last)
{
	return (tab[first] & PTE_FLAGS) == (tab[last] & PTE_FLAGS);
}

static int
next_offset(uval *tab, uval first, uval last)
{
	uval offset = (last - first) * PGSIZE;

	return ((tab[first] & PTE_SMALLBASE) + offset) ==
		(tab[last] & PTE_SMALLBASE);
}

static void
dump_pgtab(uval vaddr, union pgframe *pgt, int lpar)
{
	uval *tab = lpar ? pgt->pgtab.lp_vaddr : pgt->pgtab.hv_vaddr;
	uval pti;
	uval first = NR_MMU_PTES;

	for (pti = 0; pti < NR_MMU_PTES; pti++) {
		if ((tab[pti] & PTE_P) == 0) {
			if (first < NR_MMU_PTES) {
				dump_pte(vaddr + (first << LOG_PGSIZE),
					 tab, first, pti, 0);
				first = NR_MMU_PTES;
			}
			continue;
		}
		if (first >= NR_MMU_PTES)
			first = pti;

		if (same_flags(tab, first, pti) &&
		    next_offset(tab, first, pti))
			continue;

		dump_pte(vaddr + (first << LOG_PGSIZE), tab, first, pti, 0);
		first = NR_MMU_PTES;

	}
	if (first < NR_MMU_PTES)
		dump_pte(vaddr + (first << LOG_PGSIZE), tab, first, pti, 0);
}

void
pgc_dump(union pgframe *pgd, int lpar)
{
	uval pdi;

	hprintf("Dumping %s PGD entries\n", lpar ? "LPAR" : "HV");
	for (pdi = 0; pdi < NR_LPAR_PGTABS; pdi++) {
		uval *tab = lpar ? pgd->pgdir.lp_vaddr : pgd->pgdir.hv_vaddr;

		if ((tab[pdi] & PTE_P) == 0)
			continue;
		if (tab[pdi] & PTE_PS)
			dump_pte(pdi << LOG_PDSIZE, tab, pdi, 0, 1);
		else if (pgd->pgdir.pgt[pdi])
			dump_pgtab(pdi << LOG_PDSIZE, pgd->pgdir.pgt[pdi],
				   lpar);
		else
			hprintf("LPAR PDE[%ld] not mapped yet\n", pdi);
	}
}
#endif /* DEBUG */
