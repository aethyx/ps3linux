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
 * test of H_HYPERVISOR_DATA hcall.
 *
 */


#include <test.h>
#include <config.h>
#include <lpar.h>
#include <lib.h>
#include <hcall.h>


struct partition_info pinfo[2] = {{
	.sysid = 1,
	.sfw_tlb = 1,
	.large_page_size1 = LARGE_PAGE_SIZE64K,
	.large_page_size2 = LARGE_PAGE_SIZE16M
},};


#define NReturns 8
#define YES 1
#define NO 0

static uval ReturnValue[][8] = {
	{0, 1, 2, 3, 4, 5, 6, 7},
	{8, 9, 10, 11, 12, 13, 14, 15}
};

static void
DumpR4_R11(EightRegs t, EightRegs r)
{
	short i;

	for (i = 0; i < NReturns; i++) {
		hprintf("0x%lx:0x%lx ", *((uval *)t)++, *((uval *)r)++);
	}
}

uval
test_os(uval argc __attribute__ ((unused)),
	uval argv[] __attribute__ ((unused)))
{
	EightRegs rv;
	uval i;
	uval j;
	uval64 control;
	int equal;

	hputs("hypervisor_data:\n");
	control = 0;
	for (i = 0; i < (sizeof (ReturnValue) / sizeof (EightRegs)); i++) {
		uval ret;
		ret = hcall_hypervisor_data(rv, control);
		assert(ret != 0,
		       "FAILED: hcall_hypervisor_data()\n");

		for (equal = YES, j = 0; j < NReturns; j++) {
			if (ReturnValue[i][j] != rv[j]) {
				equal = NO;
				break;
			}
		}

		if (!equal) {
			hprintf("%lu: control R4-R11 ", i);
			DumpR4_R11(ReturnValue[i], rv);
			hputs("\n");
			break;
		}
	}

	if (hcall_hypervisor_data(rv, control) != H_Parameter) {
		hputs("does not returs H_Parameter when beyond the range.\n");
	}
	if (hcall_hypervisor_data(rv, -3) != H_Parameter) {
		hputs("does not returs H_Parameter when called with -3.\n");
	}
	return 0;
}

