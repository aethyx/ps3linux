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


struct partition_info pinfo[2] = {{
	.sfw_tlb = 1,
	.large_page_size1 = LARGE_PAGE_SIZE16M,
	.large_page_size2 = LARGE_PAGE_SIZE64K
},};


uval
test_os(uval argc __attribute__ ((unused)),
	uval argv[] __attribute__ ((unused)))
{
	uval flags = H_ZERO_PAGE;
	uval ret2[2];
	uval retcode;
	uval destination = 0x3000, source = 0x2000;

        retcode = hcall_page_init(ret2, flags, destination, source);
	if (retcode) {
		hputs("H_PAGE_INIT: FAILURE\n");
		return retcode;
        }

	hputs("H_PAGE_INIT: SUCCESS\n");
	return 0;
}

