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
 * Test of put_term_char and get_term_char hcalls.
 *
 */

#include <test.h>


struct partition_info pinfo[2] = {{
	.sfw_tlb = 1,
	.large_page_size1 = LARGE_PAGE_SIZE64K,
	.large_page_size2 = LARGE_PAGE_SIZE16M
},};


uval
test_os(uval argc __attribute__ ((unused)),
	uval argv[] __attribute__ ((unused)))
{
	int loop_count;

	/* Pessimistically declaring a 5 length array. first ret arg is the
	   number of bytes read. On 32 bit platforms, it may return as many as 
	   4 uval's */
	uval ret_vals[5], *rets, in_bytes;
	char *ret_chars;

	loop_count=0;

	/* hcall_put_term_string is a wrapper around the 32/64 bit versions of
	   hcall_put_term_char */
	hcall_put_term_string(0, 28, "h_put_term_char: SUCCESS\n\n");

	hputs("h_get_term_char: Input five characters in console window:\n");

	while (loop_count<5) {
		hcall_get_term_char(ret_vals, 0);
		in_bytes=0;

		/* We need this if statement because hcall_get_term_char is 
		   non-blocking. Hence the loop_count mess */

		if ((in_bytes=ret_vals[0])!=0) {
			rets=ret_vals;
			rets++;
			loop_count++;
			ret_chars=(char *)rets;
			hcall_put_term_string(0, in_bytes, ret_chars);
		}
	}

	return 0;
}

