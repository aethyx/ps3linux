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

#ifndef _HYP_POWERPC_64_HTAB_H
#define _HYP_POWERPC_64_HTAB_H

#include <config.h>
#include <types.h>
#include_next <htab.h>

/* hv maintains a PTE eXtension which associates other bits of data
 * with a PTE, and these are kept in a shadow array.  Currently this
 * value is the logical page number used to create the PTE.
 */

struct logical_htab {
	uval sdr1;
	uval num_ptes;		/* number of PTEs in HTAB. */
	uval *shadow;		/* idx -> logical translation array */
};

extern void pte_insert(struct logical_htab *pt, uval ptex,
		       const uval64 vsidWord, const uval64 rpnWord, uval lrpn);

/* pte_invalidate:
 *	pt: extended hpte to use
 *	pte: PTE to invalidate
 */
static inline void
pte_invalidate(struct logical_htab *pt, union pte *pte)
{
	union pte *base = (union pte *)(pt->sdr1 & SDR1_HTABORG_MASK);
	uval ptex = ((uval)pte - (uval)base) / sizeof (union pte);

	pte->bits.v = 0;
	pt->shadow[ptex] = 0;
}

#endif
