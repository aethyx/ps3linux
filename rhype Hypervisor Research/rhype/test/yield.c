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
 */
#include <test.h>
#include <lpar.h>
#include <hypervisor.h>
#include <hcall.h>


struct partition_info pinfo[2] = {{
	.sfw_tlb = 1,
	.large_page_size1 = LARGE_PAGE_SIZE64K,
	.large_page_size2 = LARGE_PAGE_SIZE16M
},};


uval64 ctxt_count = 0;

#ifdef CPU_4xx
extern uval decrementer_rrupt;  /* a lie, but it is in assembly... */
uval ev[16] = {
	0x0100,
	0x0200,
	0x0300,
	0x0400,
	0x0500,
	0x0600,
	0x0700,
	0x0800,
	0x0900,
	0x1000,
	(uval)&decrementer_rrupt,
	0x1200,
	0x1300,
	0x1400,
	0x1500,
	0x1600
};
#endif /* CPU_4xx */

#ifdef HAS_64BIT
extern uval textvec;	/* Lie for the assembler. */
#endif

uval
test_os(uval argc __attribute__ ((unused)),
	uval argv[] __attribute__ ((unused)))
{
	uval chars[10];
	uval mylpid;
	uval regsave[32];

	(void)hcall_get_lpid(&mylpid);

	ctxt_count = ((uval64)mylpid) << 32;
	hputs("You should see repeated 'SUCCESS' messages.\n"
	      "This test will run until manually halted.\n");

	while (1) {
		sval ret;
		ctxt_count += 1;
		if ((ctxt_count & 0x0f) == 0) {
			hprintf("0x%lx: (0x%lx) yield_test: SUCCESS\n",
				mylpid, decr_cnt);
		}
		ret = hcall_yield_check_regs(mylpid, regsave);
		if (ret != H_Success) {
			hprintf("%lx = yield_test(0x%lx, %ld) FAILURE\n",
				ret, mylpid, regsave[0]);
			return ret;
		}
		if ((hcall_get_term_char(chars, 0) == H_Success) &&
		    (chars[0] != 0)) {
			hcall_put_term_string(0, chars[0], (char *)&chars[1]);
		}
	}

	return 0;
}
