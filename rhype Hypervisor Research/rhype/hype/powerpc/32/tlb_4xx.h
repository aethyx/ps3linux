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
 * Description: structures, macros, and constants specific to PPC 4xx
 *              TLB support.
 *
 * XXX rename me to tlb_44x.h - 405 has slightly different TLBs
 */
#ifndef _HYP_TLB_H
#define _HYP_TLB_H

#include <config.h>
#include <hypervisor.h>
#include <types.h>
#include <asm.h>
#include <os.h>

#define MMUCR_STID_MASK	0xff

#define MIN_OS_TLBX 3

static __inline__ uval
get_mmucr(void)
{
	uval ret;

	/* *INDENT-OFF* */
	__asm__ ("mfspr %0,%1"
			: "=r"(ret)
			: "i"(SPRN_MMUCR));
	/* *INDENT-ON* */

	return ret;
}

static __inline__ void
set_mmucr(uval mmucr)
{
	/* *INDENT-OFF* */
	__asm__ ("mtspr %1,%0"
			:
			: "r"(mmucr), "i"(SPRN_MMUCR));
	/* *INDENT-ON* */
}

/* NOTE: callers *must* have already set MMUCR:STID! */
static __inline__ void
tlbwe(uval tlbx, uval word0, uval word1, uval word2)
{
	/* *INDENT-OFF* */
	__asm__ (
		"tlbwe %1,%0,0\n"
		"tlbwe %2,%0,1\n"
		"tlbwe %3,%0,2\n"
		:
		: "r"(tlbx), "r"(word0), "r"(word1), "r"(word2)
		: "memory"
		);
	/* *INDENT-ON* */
}

/* NOTE: callers must retrieve TID from MMUCR:STID */
static __inline__ void
tlbre(uval tlbx, uval *word0, uval *word1, uval *word2)
{
	/* *INDENT-OFF* */
	__asm__ (
#if 0
		"tlbre %0,%3,0\n"
		"tlbre %1,%3,1\n"
		"tlbre %2,%3,2\n"
#else
		/* XXX for assemblers which can't handle tlbre */
		".long (31<<26)+(%0<<21)+(%3<<16)+(0<<11)+(946<<1)\n"
		".long (31<<26)+(%1<<21)+(%3<<16)+(1<<11)+(946<<1)\n"
		".long (31<<26)+(%2<<21)+(%3<<16)+(2<<11)+(946<<1)\n"
#endif
		: "=&r"(*word0), "=&r"(*word1), "=&r"(*word2)
		: "r"(tlbx)
		);
	/* *INDENT-ON* */
}

/* NOTE: callers *must* have already set MMUCR:STID and MMUCR:STS! */
static __inline__ uval
tlbsx(uval eaddr)
{
	uval ret;

	/* *INDENT-OFF* */
	__asm__ (
		"tlbsx. %0,0,%1\n\t"
		"beq	1f\n\t"
		"li	%0,-1\n"
		"1:"
		: "=r"(ret)
		: "r"(eaddr)
		: "cr0");
	/* *INDENT-ON* */

	return ret;
}

#endif
