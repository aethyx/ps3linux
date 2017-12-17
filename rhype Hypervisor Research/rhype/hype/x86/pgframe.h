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
#ifndef __HYPE_X86_PGFRAME_H__
#define __HYPE_X86_PGFRAME_H__

/* PGC_PAEDIR = 0 is another (potential) type */
enum pgc_type { PGC_PGDIR = 0, PGC_PGTAB, PGC_NTYPES };

/*
 * Page frames
 */
union pgframe {
	/*
	 * These fields are common among all frame types
	 */
	struct pgcommon {
		union pgframe *next;	/* next in chain */
		uval lp_laddr;	/* LPAR page (logical) */
		uval *lp_vaddr;	/* LPAR page (HV virtual) */
		uval *hv_vaddr;	/* shadow page  */
		uval hv_paddr;	/* PA of shadow page */
	} cmn;

	/*
	 * PGC_PGDIR type frame: Per pgdir we keep the information
	 * below, 1 page holding the shadow page directory. and 1
	 * page holding a directory of pointers to the pgtab frames.
	 */
	struct pgdir {
		union pgframe *next;	/* next in chain */
		uval lp_laddr;	/* LPAR page directory (logical) */
		uval *lp_vaddr;	/* LPAR page directory (HV virtual) */
		uval *hv_vaddr;	/* shadow page directory (4KB) */
		uval hv_paddr;	/* PA of shadow page directory */
		uval last;	/* time of last context switch */
		union pgframe **pgt;	/* pointers to pgtab's */
	} pgdir;

	/*
	 * PGC_PGTAB type frame: Per pgtab we keep the information below
	 * and the actual shadow page table.
	 */
	struct pgtab {
		union pgframe *next;	/* next in chain */
		uval lp_laddr;	/* LPAR page table (logical) */
		uval *lp_vaddr;	/* LPAR page table (HV virtual) */
		uval *hv_vaddr;	/* shadow page table (4KB) */
		uval hv_paddr;	/* PA of shadow page table */
		uval pdi;	/* page directory index */
		union pgframe *pgd;	/* backlink to page directory */
	} pgtab;

	/*
	 * Ensure that these structures are 32 byte aligned
	 */
	uval8 align[32];
};

#define	HASHSIZE	32
#define	HASH(laddr)	(((laddr) >> 12) & (HASHSIZE - 1))

/*
 * Page frame cache
 */
struct pgcache {
	union pgframe *frames;
	uval freecount[PGC_NTYPES];
	union pgframe *freelist[PGC_NTYPES];
	union pgframe *htab[PGC_NTYPES - 1][HASHSIZE];
};

#endif /* __HYPE_X86_PGFRAME_H__ */
