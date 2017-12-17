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

#ifndef _POWERPC_64_HTAB_H
#define _POWERPC_64_HTAB_H

#include <config.h>
#include <types.h>
#include_next <htab.h>

#define SDR1_HTABORG_MASK	0xfffffffffff80000ULL
#define SDR1_HTABSIZE_MASK	0x1fUL
#define SDR1_HTABSIZE_MAX	46
#define SDR1_HTABSIZE_BASEBITS	11

/* used to turn a vsid into a number usable in the hash function */
#define VSID_HASH_MASK		0x0000007fffffffffUL

/* used to turn a vaddr into an api for a pte */
#define VADDR_TO_API(vaddr)  	(((vaddr) & API_MASK) >> API_SHIFT)
#define API_VEC   		0x1fUL
#define API_SHIFT 		23
#define API_MASK  		(API_VEC << API_SHIFT)

union pte {
	struct pte_words {
		uval64 vsidWord;
		uval64 rpnWord;
	} words;
	struct pte_bits {
		/* *INDENT-OFF* */
		/* high word */
		uval64 avpn:	57;	/* abbreviated virtual page number */
		uval64 lock:	1;	/* hypervisor lock bit */
		uval64 res:	1;	/* reserved for hypervisor */
		uval64 bolted:	1;	/* XXX software-reserved; temp hack */
		uval64 sw:	1;	/* reserved for software */
		uval64 l:	1;	/* Large Page */
		uval64 h:	1;	/* hash function id */
		uval64 v:	1;	/* valid */

		/* low word */
		uval64 pp0:	1;	/* page protection bit 0 (current PPC
					 *   AS says it can always be 0) */
		uval64 ts:	1;	/* tag select */
		uval64 rpn:	50;	/* real page number */
		uval64 res2:	2;	/* reserved */
		uval64 ac:	1;	/* address compare */
		uval64 r:	1;	/* referenced */
		uval64 c:	1;	/* changed */
		uval64 w:	1;	/* write through */
		uval64 i:	1;	/* cache inhibited */
		uval64 m:	1;	/* memory coherent */
		uval64 g:	1;	/* guarded */
		uval64 n:	1;	/* no-execute */
		uval64 pp1:	2;	/* page protection bits 1:2 */
		/* *INDENT-ON* */
	} bits;
};

union ptel {
	uval64 word;
	struct ptel_bits {
	    /* *INDENT-OFF* */

		uval64 pp0:	1;	/* page protection bit 0 (current PPC
					 *   AS says it can always be 0) */
		uval64 ts:	1;	/* tag select */
		uval64 rpn:	50;	/* real page number */
		uval64 res2:	2;	/* reserved */
		uval64 ac:	1;	/* address compare */
		uval64 r:	1;	/* referenced */
		uval64 c:	1;	/* changed */
		uval64 w:	1;	/* write through */
		uval64 i:	1;	/* cache inhibited */
		uval64 m:	1;	/* memory coherent */
		uval64 g:	1;	/* guarded */
		uval64 n:	1;	/* no-execute */
		uval64 pp1:	2;	/* page protection bits 1:2 */
		/* *INDENT-ON* */
	} bits;
};

#endif
