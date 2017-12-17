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
 * "Smoke" test of an OS yielding when there is no other OS to yield to.
 *
 */
#include <test.h>
#include <lpar.h>
#include <hypervisor.h>
#include <hcall.h>


struct partition_info pinfo[2] = {{
	.sfw_tlb = 1,
	.large_page_size1 = LARGE_PAGE_SIZE16M,
	.large_page_size2 = LARGE_PAGE_SIZE64K
},};


const char str1[] = "Thread 1.\n";
const char str2[] = "Thread 2.\n";

uval
test_os(uval argc __attribute__ ((unused)),
	uval argv[] __attribute__ ((unused)))
{
	int i = 0;
	const char *string;
	string = str2;

	if (mfpir() == 0)
	{
	    string = str1;
	    hcall_put_term_string(0, 10, string);
	    hcall_thread_control(NULL, HTC_START, 1, 0x100);
	}
	else
	{
	    hcall_put_term_string(0, 10, string);
	}

	while (1) {
	    ++i;
	    if ((i % 25000) == 0)
	    {
		hcall_put_term_string(0, 10, string);
	    }
	}
}

