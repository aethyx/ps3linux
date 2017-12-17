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
 *
 $Id$
 */

#include <config.h>
#include <hypervisor.h>
#include <mmu.h>
#include <hype.h>
#include <os.h>
#include <h_proto.h>
#include "tlb_4xx.h"
#include "tlbe_44x.h"

/* GT_4GB
 * addr + offset > 4GB ? check with 32-bit arithmetic.
 * assumes offset is non-0!
 */
#define GT_4GB(addr, offset)	(0x00000000UL - offset < eaddr)

/* last TLB index used by *MapRange(). always increases */
static int last_tlbx = 10;

enum {
	TLBSIZE_1KB,
	TLBSIZE_4KB,
	TLBSIZE_16KB,
	TLBSIZE_64KB,
	TLBSIZE_256KB,
	TLBSIZE_1MB,
	TLBSIZE_16MB,
	TLBSIZE_256MB
};

struct tlbsize {
	uval numbytes;
	uval SIZE;
	uval log2_SIZE;
	uval RPN_mask;
};

/* @@@ What the heck can you do with log2_SIZE??? */

/* *INDENT-OFF* */
static const struct tlbsize tlb_sizes[8] = {
	/* numbytes,	SIZE,	log2(SIZE),	RPN mask */
	{ 0x400,	0x0,	0,	~0x0 }, /* 1KB */
	{ 0x1000,	0x1,	1,	~0x3 },	/* 4KB */
	{ 0x4000,	0x2,	2,	~0x7 },	/* 16KB */
	{ 0x10000,	0x3,	2,	~0x1f },	/* 64KB */
	{ 0x40000,	0x4,	3,	~0x7f },	/* 256KB */
	{ 0x100000,	0x5,	3, 	~0x1ff },	/* 1MB */
	{ 0x1000000,	0x7,	3, 	~0x1fff },	/* 16MB */
	{ 0x10000000,	0x9,	4,	~0x1ffff },	/* 256MB */
};
/* *INDENT-On* */

/* private functions */

static void
init_tlbe(union tlbe *tlbep, uval eaddr, uval64 raddr, int tlbsize)
{
	/* initialize TLBE */
	tlbep->words.epnWord = 0;
	tlbep->words.rpnWord = 0;
	tlbep->words.attribWord = 0;
	tlbep->bits.tid = 0x0;

	/* @@@ The "10" below should be symbolic. */
	tlbep->bits.epn = (eaddr >> 10) & tlb_sizes[tlbsize].RPN_mask;
	tlbep->bits.v = 1;
	tlbep->bits.size = tlb_sizes[tlbsize].SIZE;
	tlbep->bits.rpn = (raddr >> 10) & tlb_sizes[tlbsize].RPN_mask;
	tlbep->bits.erpn = raddr >> 32;
	tlbep->bits.sp = 0x7; /* all permissions */
}

/* public functions */

/* hwMapRange()
 * Maps an effective range into the hardware UTLB.
 *
 *	eaddr - starting effective address
 *	raddr - starting real address
 *	size - length in bytes of region to map
 *	bolted - meaningful only for ppc64 HTAB, ignored here.
 */
void hwMapRange(uval eaddr, uval64 raddr, uval size, /* unused */ int bolted)
{
	uval offset;
	int pagesize = TLBSIZE_16MB;  /* Remember, must be aligned!!! */

	assert(((raddr >> 36) == 0),
	       "raddr: %0xull: beyond real address space of 2^36\n",
	       raddr);

	if (size == 0)
		return;

	/*
	 * make sure we're not crossing the 4GB limit. If you need more
	 * you must call again with different ERPN (eaddr bits 32:35)
	 */
	assert(!(GT_4GB(eaddr, size) || GT_4GB(raddr, size)),
	       "hwMapRange tried to map >4GB", -1);

#ifdef DEBUG
	hprintf("hwMapRange bytes: 0x%lx\n"
		"  raddr: 0x%lx\n"
		"  eaddr: 0x%lx\n",
		size, raddr, eaddr);
#endif

	for (offset = 0; offset < size;
	     offset += tlb_sizes[pagesize].numbytes) {
		union tlbe tlbe; /* this page's TLB entry */
		init_tlbe(&tlbe, eaddr + offset, raddr + offset, pagesize);

		/* XXX what to do with TID? */
		set_mmucr(get_mmucr() & ~MMUCR_STID_MASK); /* MMUCR:STID = 0 */
		tlbwe(last_tlbx++, tlbe.words.epnWord, tlbe.words.rpnWord,
				tlbe.words.attribWord);
	}
}
