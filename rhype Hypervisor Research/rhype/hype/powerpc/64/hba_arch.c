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

#include <config.h>
#include <hba.h>
#include <mmu.h>
#include <lib.h>
#include <os.h>
#include <pmm.h>
#include <logical.h>
#define HBA_NUM 1

static uval hba_amo_check_WIMG(struct logical_chunk_info *, uval, struct ptel);

struct arch_mem_ops hba_ops = {
	.amo_sav_ptep = NULL,
	.amo_rem_ptep = NULL,
	.amo_check_WIMG = hba_amo_check_WIMG,
};

static uval
hba_amo_check_WIMG(struct logical_chunk_info *lci, uval laddr,
		   struct ptel ptel)
{
	(void *)lci;
	/* b01*1 is valid */
	if ((ptel.bits.w == 0) && (ptel.bits.i == 1) && (ptel.bits.g == 1)) {
		return 1;
	}
#ifdef DEBUG
	hprintf("hba: WIMG bits incorrect for memory type "
		"w=%x i=%d m=%d, g=%d\n word 0x%llx\n", ptel.bits.w,
		ptel.bits.i, ptel.bits.m, ptel.bits.g, ptel.word);
#endif
	return 0;
}
