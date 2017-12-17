/*
 * Copyright (C) 2005 Michal Ostrowski <mostrows@watson.ibm.com>, IBM Corp.
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
#ifndef __POWERPC_MMU_DEFS_H_
#define __POWERPC_MMU_DEFS_H_


#define BOOT_STACK_SIZE (20 * PGSIZE)

#ifdef CPU_4xx
#define LOG_PGSIZE 10		/* XXX what? 440 page sizes are variable... */
#elif HAS_64BIT
#define LOG_PGSIZE 12
#endif

#define PGSIZE		((1ULL) << LOG_PGSIZE)
#define PGMASK		(~(PGSIZE-1))
#define PGALIGN(a)	ALIGN_UP(a, PGSIZE)
/* returns number of pages for a size s */
#define PGNUMS(s)	(PGALIGN(s) >> LOG_PGSIZE)

#define LOG_LARGE_PGSIZE 24
#define LARGE_PGSIZE ((1ULL) << LOG_LARGE_PGSIZE)

/* pte bits */
#define PAGE_WIMG_G	0x008UL	/* G: Guarded */
#define PAGE_WIMG_M	0x010UL	/* M: enforce memory coherence */
#define PAGE_WIMG_I	0x020UL	/* I: cache inhibit */
#define PAGE_WIMG_W	0x040UL	/* W: cache write-through */

#endif /* !__POWERPC_MMU_DEFS_H_ */
