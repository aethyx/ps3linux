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
#include <mmu.h>

struct partition_info pinfo[2] = {{
	.sfw_tlb = 1,
	.large_page_size1 = LARGE_PAGE_SIZE64K,
	.large_page_size2 = LARGE_PAGE_SIZE16M
},};


uval
test_os(uval argc __attribute__ ((unused)),
	uval argv[] __attribute__ ((unused)))
{
	volatile uval *fooaddr;
	volatile uval foo;

	/* insert an SLB entry for our test segment */
        slb_insert(0xf0000000, 1, 1); 
	/* insert a PTE for out test segment */
	pte_enter(0xf0000000, 0x00000000, PAGE_WIMG_M, 0, 1, 1, &pinfo[1]);

	fooaddr = (uval*)0x00000000;
	foo = *fooaddr;
	hprintf("foo=0x%lx\n", foo);

	hputs("Touching 0xf0000000\n");
	fooaddr = (uval*)0xf0000000;
	foo = *fooaddr;
	hprintf("foo=0x%lx\n", foo);
	hputs("Touched 0xf0000000\n");

	pte_remove(0xf0000000, 0x00000000, 1, 1);
	hputs("Removed PTE\n");


	hputs("Touching 0xf0000000 - test is a success if a 0x300 exception is taken\n");
	fooaddr = (uval*)0xf0000000;
	foo = *fooaddr;
	hprintf("foo=0x%lx\n", foo);
	hputs("Touched 0xf0000000\n");

	return 0;
}

