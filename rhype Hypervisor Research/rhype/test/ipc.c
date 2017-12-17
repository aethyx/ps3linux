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
 * Common infrastructure for receiving async IPC messages
 *
 */
#include <test.h>
#include <lpar.h>
#include <hypervisor.h>
#include <hcall.h>
#include <xh.h>
#include <sysipc.h>

char buf[8192] __attribute__((aligned(4096)));
struct msg_queue* mq = (struct msg_queue*)buf;
void (*aipc_msgHandler)(struct async_msg_s *msg);

uval
aipc_handler(uval ex __attribute__ ((unused)),
	     uval *regs __attribute__ ((unused)))
{
#ifdef AIPC_DEBUG
	hprintf_nlk("\n%s: h: 0x%lx, t: 0x%lx\n",
		    __func__, mq->head, mq->tail);
#ifdef XHR_SRR0
	hprintf_nlk("\n%s: %lx %lx \n",
		    __func__, regs[XHR_SRR0], regs[XHR_SRR1]);
#endif
#endif
	uval retval[1] = {0};
	sval rc = 0;
	while ((rc = hcall_xirr(retval)) == H_Success) {
		if (retval[0] <= 0) break;

#ifdef AIPC_DEBUG
		hprintf_nlk("AIPC interrupt: %ld %lx\n", rc, retval[0]);
#endif

		while (mq->tail < mq->head) {
			struct async_msg_s *am =
				&mq->buffer[mq->tail % mq->bufSize];
			mq->tail = mq->tail + 1;
			(*aipc_msgHandler)(am);
#ifdef AIPC_DEBUG
			hprintf_nlk("\n%s: from %lx [%lx %lx %lx %lx]\n",
				    __func__,
				    am->source,
				    am->am_data.amu_data[0],
				    am->am_data.amu_data[1],
				    am->am_data.amu_data[2],
				    am->am_data.amu_data[3]);
#endif
		}
		hcall_eoi(NULL, retval[0]);
	}
	return 0;
}

int
aipc_config(void (*handler)(struct async_msg_s *msg))
{
	int ret;
	uval mylpid;
	uval retval[1];
	(void)hcall_get_lpid(&mylpid);

	aipc_msgHandler = handler;
	hprintf("Creating AIPC buffer at %p for %ld\n", buf, mylpid);

	ret = hcall_create_msgq(retval, (uval)buf, 8192, 0);
	if (ret != H_Success) {
		hprintf("Failed AIPC create: %d\n", ret);
	}
	hprintf("Create msgq: %lx\n", retval[0]);
	hcall_interrupt(NULL, retval[0], 1);

	return ret;
}
