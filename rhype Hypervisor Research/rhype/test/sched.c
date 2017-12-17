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
#define THIS_CPU	0xffff


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
	uval mylpid;
	uval counter = 0;
	uval schedvals[3];
	sval rotate;

	(void)hcall_get_lpid(&mylpid);

	if (mylpid == 0x1001) {
		rotate = hcall_set_sched_params(schedvals, H_SELF_LPID,
						THIS_CPU, 0x00110011,0);
		hprintf("Scheduler for 0x%lx: rot: "
			"%ld (%016lx, %016lx %016lx)\n",
			mylpid, rotate, schedvals[0],
			schedvals[1], schedvals[2]);

		rotate = hcall_set_sched_params(schedvals, H_SELF_LPID,
						THIS_CPU, 0x10001000,0);
		hprintf("Scheduler for 0x%lx: rot: "
			"%ld (%016lx, %016lx %016lx)\n",
			mylpid, rotate, schedvals[0],
			schedvals[1], schedvals[2]);

		rotate = hcall_set_sched_params(schedvals, 0x1002,
						THIS_CPU, 0x01000100,0);
		while (rotate < 0) {
			hcall_yield(NULL, H_SELF_LPID);
			rotate = hcall_set_sched_params(schedvals, 0x1002,
							THIS_CPU,
							0x01000100,0);
		}
		hprintf("Scheduler for 0x%lx: rot: "
			"%ld (%016lx, %016lx %016lx)\n",
			mylpid, rotate, schedvals[0],
			schedvals[1], schedvals[2]);

		rotate = hcall_set_sched_params(schedvals, H_SELF_LPID,
						THIS_CPU, 0x10000000,0);
		hprintf("Scheduler for 0x%lx: rot: "
			"%ld (%016lx, %016lx %016lx)\n",
			mylpid, rotate, schedvals[0],
			schedvals[1], schedvals[2]);
	}
	ctxt_count = ((uval64)mylpid) << 32;
	hputs("You should see repeated 'SUCCESS' messages.\n"
	      "This test will run until manually halted.\n");

	while (1) {
		++counter;
		if ((counter & 0xffff) == 0xffff) {
			++counter;
			hprintf("0x%lx: (0x%lx) counter: 0x%lx\n",
			 	mylpid, decr_cnt,counter>>16);
		}
	}

	return 0;
}
