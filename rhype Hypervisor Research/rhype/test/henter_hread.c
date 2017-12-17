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

#include <test.h>
#include <config.h>
#include <asm.h>
#include <lpar.h>
#include <types.h>
#include <sim.h>
#include <lib.h>
#include <hcall.h>


struct partition_info pinfo[2] = {{
	.large_page_size1 = LARGE_PAGE_SIZE16M,
	.large_page_size2 = LARGE_PAGE_SIZE64K
},};


uval
test_os(uval argc __attribute__ ((unused)),
	uval argv[] __attribute__ ((unused)))
{
	uval pte_high, pte_low;
	uval flags = 0;
	uval pteg_index = 0, pte_index;
	uval ret2[2];
	uval retcode;

	pte_high = 0x000000000801;
	pte_low  = 0x000000040002;

	retcode = hcall_enter(ret2, flags, pteg_index, pte_high, pte_low);
	if (retcode) {
		hprintf("H_ENTER: FAILURE: returned 0x%lx\n", retcode);
		return retcode;
	}
	pte_index = ret2[0];

	retcode = hcall_read(ret2, flags, pte_index);
	if (retcode) {
		hprintf("H_READ: FAILURE: returned 0x%lx\n", retcode);
		return retcode;
	}

	/* Test that registers entered into page table match what was read out */
	if (pte_high != ret2[0] || pte_low != ret2[1]) {
		hprintf("H_ENTER/H_READ: FAILURE: expected 0x%lx: 0x%lx\n"
			"  read: 0x%lx : 0x%lx\n",
			pte_high, pte_low, ret2[0], ret2[1]);
		return -1;
	}

	hputs("H_ENTER/H_READ: SUCCESS\n");
	return 0;
}

