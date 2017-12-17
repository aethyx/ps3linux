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

#include <lib.h>
#include <pmm.h>
#include <ipc.h>
#include <hype.h>
#include <aipc.h>
#include <xirr.h>
#include <xir.h>
#include <objalloc.h>
#include <hv_regs.h>
#include <vio.h>

static struct xh_data xhd_aipc[XIRR_DEVID_SZ];

static uval
aipc_cleanup(struct os *os, struct lpar_event *le, uval event)
{
	(void)event;

	/* Called in partition destruction, so it's safe not to lock */
	uval i = 0;

	for (; i < MAX_NUM_AIPC; ++i) {
		if (!os->aipc_srv.aipc[i])
			continue;

		hfree(os->aipc_srv.aipc[i], sizeof (struct aipc));
		os->aipc_srv.aipc[i] = NULL;
	}
	hfree(le, sizeof (*le));
	return 0;
}

sval
aipc_create(struct cpu_thread *thread, uval lbase, uval size, uval flags)
{
	uval rbase;
	sval ret = H_Success;

	if ((size | lbase) & (PGSIZE - 1)) {
		return H_INVAL;
	}

	/* Buffer must be in first logical chunk */
	if (lbase + size > CHUNK_SIZE) {
		return H_Parameter;
	}

	struct os *os = thread->cpu->os;

	lock_acquire(&os->po_mutex);

	uval i = 0;

	for (; i < MAX_NUM_AIPC; ++i) {
		if (!os->aipc_srv.aipc[i])
			break;
	}
	if (i == MAX_NUM_AIPC) {
		goto abort;
	}

	rbase = hv_map_LA(os, lbase, PGSIZE);

	if (rbase == INVALID_PHYSICAL_ADDRESS) {
		ret = H_Parameter;
		goto abort;
	}

	memset((void *)rbase, 0, size);

	struct aipc *aipc = halloc(sizeof (struct aipc));

	aipc->buf = (struct msg_queue *)rbase;
	aipc->buf_size = (size - sizeof (struct msg_queue)) /
		sizeof (struct async_msg_s);

	aipc->buf->bufSize = aipc->buf_size;
	lock_init(&aipc->lock);

	ret = xir_find(XIRR_CLASS_AIPC);
	assert(ret != -1, "Can't find AIPC vector\n");
	aipc->xirr = xirr_encode(ret, XIRR_CLASS_AIPC);

	xir_default_config(aipc->xirr, thread, aipc);

	os->aipc_srv.aipc[i] = aipc;

	if (!os->aipc_srv.cleanup) {
		/* First AIPC , configure cleanup */
		os->aipc_srv.cleanup = halloc(sizeof (struct lpar_event));
		os->aipc_srv.cleanup->le_mask = LPAR_DIE;
		os->aipc_srv.cleanup->le_func = &aipc_cleanup;
		register_event(os, os->aipc_srv.cleanup);
	}

	if (flags & AIPC_LOCAL) {
		if (!os->aipc_srv.aipc[thread->local_aipc_idx] ||
		    os->aipc_srv.aipc[thread->local_aipc_idx]->target
		    != thread) {
			thread->local_aipc_idx = i;
		}
		aipc->target = thread;
	}
/* *INDENT-OFF* */
abort:
/* *INDENT-ON* */

	lock_release(&os->po_mutex);

	return ret;

}

static sval
__ipc_send(struct cpu_thread *dst_thr,
	   struct aipc *aipc, uval sender,
	   uval arg1, uval arg2, uval arg3, uval arg4)
{
	struct msg_queue *mqp;
	struct async_msg_s *msg;
	uval mqbufsize;
	uval err = 0;
	uval headp;

	if (aipc->buf == NULL) {
		return H_Parameter;
	}

	lock_acquire(&aipc->lock);

	mqp = aipc->buf;
	mqbufsize = aipc->buf_size;

	/*
	 * Get the head pointer, see if space is available,
	 * if so increment and copy_out the head pointer
	 */
	if (mqp->head >= mqp->tail + mqbufsize) {
		err = H_Busy;
		goto unlock;
	}
	headp = mqp->head;

	/* copy out the actual message */
	msg = &mqp->buffer[headp % mqbufsize];
	msg->am_source = sender;
	msg->am_data.amu_data[0] = arg1;
	msg->am_data.amu_data[1] = arg2;
	msg->am_data.amu_data[2] = arg3;
	msg->am_data.amu_data[3] = arg4;

	++mqp->head;
	if (mqp->head == mqp->tail + 1) {
		int ret = xir_raise(aipc->xirr, &dst_thr);

		assert(ret >= 0, "xirr queue full\n");
	}
/* *INDENT-OFF* */
unlock:
/* *INDENT-ON* */

	lock_release(&aipc->lock);
	return err;
}

sval
ipc_directed_aipc(struct os *dest, uval aipc_id,
		  uval src, uval arg1, uval arg2, uval arg3, uval arg4)
{
	if (aipc_id >= MAX_NUM_AIPC)
		return H_Parameter;

	struct aipc *dst = dest->aipc_srv.aipc[aipc_id];

	if (!dst) {
		return H_Parameter;
	}

	struct cpu_thread *dst_thr = dst->target;

	if (!dst_thr) {
		dst_thr = get_os_thread(dest, get_proc_id(), get_thread_id());
	}
	if (!dst_thr) {
		return H_Parameter;
	}

	assert(dst_thr->cpu->os == dest, "Bad os/thread");

	return __ipc_send(dst_thr, dst, src, arg1, arg2, arg3, arg4);
}

sval
ipc_send_async(struct cpu_thread *thread, uval sender,
	       uval arg1, uval arg2, uval arg3, uval arg4)
{
	struct aipc *aipc;

	if (thread == NULL) {
		return H_Parameter;
	}

	aipc = thread->cpu->os->aipc_srv.aipc[thread->local_aipc_idx];

	if (!aipc) {
		return H_Parameter;
	}

	return __ipc_send(thread, aipc, sender, arg1, arg2, arg3, arg4);
}

static struct vios aipc_vios = {
	.vs_name = "aipc",
};

uval
aipc_sys_init(void)
{

	if (vio_register(&aipc_vios, HVIO_AIPC) == HVIO_AIPC) {
		lock_init(&xirr_classes[XIRR_CLASS_AIPC].xc_lock);
	
		xirr_classes[XIRR_CLASS_AIPC].eoi_fn = NULL;
		xirr_classes[XIRR_CLASS_AIPC].xc_data = xhd_aipc;
		return 1;
	}
	assert(0, "failed to register: %s\n", aipc_vios.vs_name);
	return 0;
}
