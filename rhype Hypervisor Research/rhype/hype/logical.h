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
 * Definitions for OS-description data structures.
 *
 */
#ifndef _LOGICAL_H
#define _LOGICAL_H
/* Generic addressable resources */
#include <config.h>
#include <atomic.h>
#include <rcu.h>
#include <arch_logical.h>
#include <bitmap.h>
#include <mmu.h>
#include <resource.h>
#include <util.h>

/* from config.h */
#define CHUNK_SIZE        (1UL << LOG_CHUNKSIZE)

/* 1 Gig - must be a multiple of CHUNK_SIZE */
#define MAX_MEM_SIZE      (1024*1024*1024)
#define MAX_MEM_CHUNKS    (MAX_MEM_SIZE / CHUNK_SIZE)

#define INVALID_PHYSICAL_ADDRESS -1UL
#define INVALID_PHYSICAL_PAGE    -1UL
#define INVALID_PHYSICAL_CHUNK   -1UL

enum {
	INVALID_CHUNK,
	VIRTUAL_CHUNK,
	EXTENDED_CHUNK,
	PHYS_MEM_CHUNK
};

#define INVALID_LOGICAL_ADDRESS  -1UL
#define INVALID_LOGICAL_PAGE     -1UL
#define INVALID_LOGICAL_CHUNK    -1UL

#define OWNER_UNOWNED -1UL
#define OWNER_HYP     -2UL

#define chunk_number_addr(i) ((i) >> LOG_CHUNKSIZE)
#define chunk_offset_addr(i) ((i) & ((1 << LOG_CHUNKSIZE) - 1))

#define chunk_number_page(i) ((i) >> (LOG_CHUNKSIZE - LOG_PGSIZE))
#define chunk_offset_page(i) ((i) & ((1 << (LOG_CHUNKSIZE - LOG_PGSIZE)) - 1))

#ifndef __ASSEMBLY__

extern struct logical_chunk_info *laddr_to_lci(struct os *os, uval laddr);

/* If lci matches the lci already set for the hint_laddr, the lci
   entry is cleared. Only detach_lci should know this. */
extern uval set_lci(struct os *os, uval hint_laddr,
		    struct logical_chunk_info *lci);

/* Flags for detach_lci */
enum {
	LCI_DETACH = 0x1,	/* Remove from logical_mmap, if applicable */
	LCI_UNMAP = 0x2,	/* Remove PTE's */
	__LCI_DESTROY = 0x4,	/* You better know what you are doing. */
	LCI_DESTROY = 0x7,	/* The right way to get rid of the thing. */
};

extern uval detach_lci(struct logical_chunk_info *lci);

struct chunk_ops {
	/* If present used for translation of logical to real */
	sval (*l_to_r) (struct logical_chunk_info * lci,
			uval laddr, uval size);

	sval (*export) (struct logical_chunk_info * lci, uval laddr, uval size,
			struct logical_chunk_info ** plci);

	/* Called when removed from logical mapping */
	sval (*detach) (struct logical_chunk_info * lci);

	/* Implies a call to detach as well */
	sval (*destroy) (struct logical_chunk_info * lci);

	sval (*unmap) (struct logical_chunk_info * lci);
};

/* Constants for lci_flags */
enum {
	LCI_DISABLED = 0x1,
	LCI_READ = 0x2,
	LCI_WRITE = 0x3,
};

/* info for each simple chunk in the logical memory map */
struct logical_chunk_info {
	/* If the DISABLED_ADDR bit is set, the LCI translation must fail */
	uval lci_raddr;		/* x-late of LA to RA, if no l_to_r */

	uval lci_laddr;
	uval lci_size;
	uval32 lci_ptecount;	/* number of PTE's */
	uval32 lci_flags;
	struct chunk_ops *lci_ops;

	/* Arch specific operations */
	/* Do not make any of these LCI specific.  If an LCI is
	 * embedded inside another one (e.g. PSM) then the LCI in
	 * which is the container may use it's own lci_arch_ops
	 * and not use ours.  The rule is that all or none of the
	 * embedded LCI's lci_arch_ops are to be used.
	 */
	struct arch_mem_ops *lci_arch_ops;

	struct os *lci_os;

	struct sys_resource lci_res;
	struct rcu_node lci_rcu;
};

static inline struct logical_chunk_info *
sysres_to_lci(struct sys_resource *res)
{
	return PARENT_OBJ(struct logical_chunk_info, lci_res, res);
}

static inline void
lci_disable(struct logical_chunk_info *lci)
{
	while (!cas_uval32(&lci->lci_flags, lci->lci_flags,
			   lci->lci_flags | LCI_DISABLED)) ;
	/* This is like a lock acquisition */
	sync_after_acquire();
}

static inline void
lci_enable(struct logical_chunk_info *lci)
{
	/* This is like a lock release */
	sync_before_release();
	while (!cas_uval32(&lci->lci_flags, lci->lci_flags,
			   lci->lci_flags & ~LCI_DISABLED)) ;
}

/* Returns non-zero if laddr is handled by lci */
static inline uval
lci_owns(struct logical_chunk_info *lci, uval laddr)
{
	return range_contains(lci->lci_laddr, lci->lci_size, laddr);
}

static inline uval
lci_translate(struct logical_chunk_info *lci, uval laddr, uval size)
{
	if (lci->lci_flags & LCI_DISABLED) {
		return INVALID_PHYSICAL_ADDRESS;
	}

	if (lci->lci_raddr == INVALID_PHYSICAL_ADDRESS) {
		return lci->lci_ops->l_to_r(lci, laddr, size);
	}

	/* Range checks */
	if (!lci_owns(lci, laddr) || !lci_owns(lci, laddr + size - 1)) {
		return INVALID_PHYSICAL_ADDRESS;
	}

	return laddr - lci->lci_laddr + lci->lci_raddr;
}

extern void phys_mem_lci_init(struct logical_chunk_info *lci,
			      uval raddr, uval size);

/* logical memory management strucutures  (1G / 16M) */
enum {
#ifdef HAS_64BIT
	MAX_LOGICAL_MEM_CHUNKS = ((1UL << LOG_PGSIZE) / sizeof (void *)),
	MAX_LOGICAL_ADDRESS = (MAX_LOGICAL_MEM_CHUNKS << LOG_CHUNKSIZE),
#else
	MAX_LOGICAL_ADDRESS = (~0UL << LOG_CHUNKSIZE),
	MAX_LOGICAL_MEM_CHUNKS = (MAX_LOGICAL_ADDRESS / (1UL << LOG_CHUNKSIZE))
#endif
};

/* per-os map of logical memory */
struct logical_memory_map {
	lock_t lmm_lock;
	struct logical_chunk_info **p_lci;
	struct dlist ps_list;
	struct lbm chunk_usage;
	uval8 lmm_bits[MAX_LOGICAL_MEM_CHUNKS / 8];
};

/* translation functions */
extern uval logical_to_physical_address(struct os *os, uval address, uval len);
extern uval logical_to_physical_page(struct os *os, uval lpn);

#endif /* ! __ASSEMBLY__ */

#endif /* #ifndef _LOGICAL_H */
