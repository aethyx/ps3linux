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
#include <sysipc.h>
#include <ipc.h>


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

static void
msgHandler(struct async_msg_s *msg)
{
	switch (msg->am_data.am_mtype) {
	case IPC_LPAR_RUNNING:
		hprintf("Detected new running partition: 0x%lx\n",
			msg->am_data.amt_data.amt_running.amr_lpid);
		break;
	case IPC_LPAR_DEAD:
		hprintf("Detected destruction of partition: 0x%lx\n",
			msg->am_data.amt_data.amt_dead.amd_lpid);
		break;
	}
}

uval
test_os(uval argc __attribute__ ((unused)),
	uval argv[] __attribute__ ((unused)))
{
	sval ret;
	hprintf("Starting\n");
	ret = aipc_config(msgHandler);


	if (ret != H_Success) {
		hprintf("Failed create: %ld\n", ret);
	}

	rrupts_on();

	ret = hcall_send_async(NULL, CONTROLLER_LPID,
			       IPC_REGISTER_SELF, 0, 0, 0);
	if (ret != H_Success) {
		hprintf("Failed send: %ld\n", ret);
	}
	hprintf("Completed\n");
	while (1) {
		hcall_yield(NULL, 0);
	}
	hprintf("Completed!!!!\n");
	return 0;
}
