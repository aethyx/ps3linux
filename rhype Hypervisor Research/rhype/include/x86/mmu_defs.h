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
#ifndef _X86_MMU_DEFS_H_
#define _X86_MMU_DEFS_H_

/* Manifest constants */
#define	LOG_PGSIZE	12	/* log2(page size) */
#define	LOG_PDSIZE	22	/* log2(page directory size) */

/* Derived constants */
#define	PGSIZE		(1 << LOG_PGSIZE)	/* page size */
#define	PGMASK		(~(PGSIZE - 1))	/* page mask */
#define	LPGSIZE		(1 << LOG_PDSIZE)	/* large page size */
#define	LPGMASK		(~(LPGSIZE - 1))	/* large page mask */

/* Useful macros */
#define	PGALIGN(a)	ALIGN_UP((a), PGSIZE)	/* align up to pgsize */
#define	PGNUMS(s)	(PGALIGN(s) >> LOG_PGSIZE)	/* num of pages */

/* Index bits into the page directory and page tables */
#define	PDE_MASK	0xFFC00000
#define	PTE_MASK	0x003FF000

/*
 * Page table directories and entries follow the standard ia32 representation:
 *
 *	Page directory		An array of 32-bit page-directory entries
 *				(PDEs) contained in a 4-KByte page. Up to
 *				1024 page-directory entries can be held in
 *				a page directory.
 *
 *	Page table		An array of 32-bit page-table entries (PTEs)
 *				contained in a 4-KByte page. Up to 1024
 *				page-table entries can be held in a page
 *				table. (Page tables are not used for 4-MByte
 *				pages. These page sizes are mapped directly
 *				from one or more page directory entries)
 *
 *	Page			A 4-KByte or 4-MByte flat address space.
 *
 *	For 4MB pages, the PSE (page size extensions) flag needs to be set.
 *
 * Page directory layout definitions:
 *
 *	Bit	Comment
 *	0	Present (P) flag
 *	1	Read/write (R/W) flag
 *	2	User/supervisor (U/S) flag
 *	3	Page-level write-through (PWT) flag
 *	4	Page-level cache disable (PCD) flag
 *	5	Accessed (A) flag
 *	6	Dirty (D) flag
 *	7	Page size (PS) flag
 *	8	Global (G) flag
 *	9-11	Reserved and available-to-software bits
 *	12-31	Page-Table Base Address
 *
 * Page table layout definitions:
 *
 *	Bit	Comment
 *	0	Present (P) flag
 *	1	Read/write (R/W) flag
 *	2	User/supervisor (U/S) flag
 *	3	Page-level write-through (PWT) flag
 *	4	Page-level cache disable (PCD) flag
 *	5	Accessed (A) flag
 *	6	Dirty (D) flag
 *	7	Page Table Attribute Index
 *	8	Global (G) flag
 *	9-11	Reserved and available-to-software bits
 *	12-31	Page-Table Base Address
 */
#define	PTE_P		(1 << 0)	/* Present */
#define	PTE_RW		(1 << 1)	/* Read/Write */
#define	PTE_US		(1 << 2)	/* User/Supervisor */
#define	PTE_PWT		(1 << 3)	/* Page-level Write-Through */
#define	PTE_PCD		(1 << 4)	/* Page-level Cache Disable */
#define	PTE_A		(1 << 5)	/* Accessed */
#define	PTE_D		(1 << 6)	/* Dirty */
#define	PTE_PS		(1 << 7)	/* Page Size */
#define	PTE_PTAI	(1 << 7)	/* Page Table Attribute Index */
#define	PTE_G		(1 << 8)	/* Global */
#define	PTE_SM		(7 << 9)	/* Available-to-software bits */
#define	PTE_FLAGS	(0x00000FFF)	/* PDE/PTE flags */
#define	PTE_SMALLBASE	(~PTE_FLAGS)	/* 4KB page address */
#define PTE_LARGEBASE	0xFFC00000	/* 4MB page address */

#define	pte_flags(e)	((e) & PTE_FLAGS)
#define	pte_pageaddr(e)	((e) & PTE_SMALLBASE)

#endif /* _X86_MMU_DEFS_H_ */
