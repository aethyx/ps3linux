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

#include <crq.h>
#include <cpu.h>
#include <lpar.h>
#include <lib.h>
#include <os.h>
#include <pmm.h>
#include <xir.h>
#include <tce.h>
#include <liob.h>
#include <vio.h>
#include <objalloc.h>
#include <bitops.h>
#include <resource.h>
#include <debug.h>

/* local data structures */
struct crq_entry {
	uval64 msg_hi;
	uval64 msg_lo;
};

struct crq_partner {
	uval16 cp_flags;
	uval16 cp_num_entries;
	uval16 cp_next_entry;
	uval16 cp_owner_res;
	uval cp_dma_size;
	struct crq_entry *cp_crqs;
	struct os *cp_os;
	xirr_t cp_interrupt;
	struct tce_data cp_tce_data;
	struct vios_resource cp_res[2];
	struct os *cp_reaper;
	struct crq_partner *cp_partner;
	lock_t cp_lock;
	lock_t *cp_lock_ptr;
};

#define CRQ_FLAG_USED		1
#define CRQ_FLAG_HAS_PARTNER    2
#define CRQ_FLAG_VALID		4

/* local defines */
/* Some defines for messages and used on CRQs */
#define CRQ_EVT_CLOSE_HI        0xFF04000000000000ULL
#define CRQ_EVT_CLOSE_LO        0x0000000000000000ULL

#define CRQ_TRANS_EVT_MASK      0xFF00000000000000ULL
#define CRQ_CMD_RESP_MASK       0x8000000000000000ULL

static __inline__ struct vios_resource *
crq_getvres(struct crq_partner *cp)
{
	return &cp->cp_res[cp->cp_owner_res];
}

static __inline__ struct sys_resource *
crq_getsres(struct crq_partner *cp)
{
	struct vios_resource *res;

	res = crq_getvres(cp);

	return &res->vr_res;
}

static __inline__ struct crq_partner *
crq_get_crq(uval liobn)
{
	return (struct crq_partner *)xir_get_device(liobn);
}

static __inline__ struct os *
crq_getos(struct crq_partner *cp)
{
	return cp->cp_os;
}

static __inline__ uval
crq_flags_test(struct crq_partner *cp, uval16 flags)
{
	return (flags == (cp->cp_flags & flags));
}

static __inline__ uval
crq_check_unused(struct crq_partner *cp)
{
	return (0 == (cp->cp_flags & CRQ_FLAG_USED));
}

static __inline__ uval
crq_haspartner(struct crq_partner *cp)
{
	return crq_flags_test(cp, (CRQ_FLAG_USED | CRQ_FLAG_HAS_PARTNER));
}

static sval
crq_put_entry_raw_locked(struct crq_partner *dst, const uval8 *msg)
{
	uval ret = H_Success;

	if (crq_haspartner(dst)) {
		uval entry = dst->cp_next_entry;

		DEBUG_OUT(DBG_CRQ, "%s: Sending message to partner.\n",
			  __func__);

		copy_out(&((dst->cp_crqs)[entry].msg_hi), msg, 8 * 2);

		/*
		 * Send signal to partner OS; other layer will determine
		 * whether to actually post it.
		 */
		DEBUG_OUT(DBG_CRQ,
			  "%s: Trying to raise an interrupt for the partner "
			  "to pick up the msg.\n", __func__);

		xir_raise(dst->cp_interrupt, NULL);

		dst->cp_next_entry++;
		if (dst->cp_next_entry == dst->cp_num_entries) {
			dst->cp_next_entry = 0;
		}
	} else {
		DEBUG_OUT(DBG_CRQ,
			  "%s: There's no partner who could receive the "
			  "message!\n", __func__);
		ret = H_Closed;
	}

	return ret;
}

static sval
crq_put_entry_locked(struct crq_partner *dst, uval8 *msg)
{
	sval ret;
	uval64 *m = (uval64 *)msg;

	/* 
	 * This produces good code on x86 AND is machine-independent
	 */
	if ((*m & CRQ_TRANS_EVT_MASK) != CRQ_TRANS_EVT_MASK &&
	    (*m & CRQ_CMD_RESP_MASK) != 0x0ULL) {
		ret = crq_put_entry_raw_locked(dst, msg);
	} else {
		ret = H_Parameter;
	}
	return ret;
}

static const uval64 close_msg[2] = {
	CRQ_EVT_CLOSE_HI,
	CRQ_EVT_CLOSE_LO
};

static sval
crq_release_locked(struct crq_partner *cp)
{
	sval ret = H_Success;
	struct crq_partner *peer = cp->cp_partner;

	ret = crq_put_entry_raw_locked(peer, (const uval8 *)close_msg);

	/*
	 * We cannot clear the TCE entries because
	 * otherwise kernel modules will fail even when rmmod
	 * is done or later when insmod is done. The only one
	 * who is setting up TCE entries is the controller.
	 * Clearing TCE entries will also be a job of the controller.
	 */

	/*
	 * The partner does not have a partner anymore.
	 */
	peer->cp_flags &= ~CRQ_FLAG_HAS_PARTNER;

	cp->cp_flags &= ~(CRQ_FLAG_USED | CRQ_FLAG_VALID);

#if NEED_TO_REAP_HERE
	cp->cp_os = cp->cp_reaper;
#endif

	return ret;
}

static __inline__ sval
crq_release_internal(struct crq_partner *cp)
{
	sval ret;

	lock_acquire(cp->cp_lock_ptr);
	ret = crq_release_locked(cp);
	lock_release(cp->cp_lock_ptr);

	/*
	 * Cannot call this function, because ther are tce_puts coming
	 * even after crq_free_crq has been called!!!
	 *
	 * xir_default_config(xenc, NULL, NULL);
	 */
	return ret;
}

static void *
crq_addr_lookup(struct crq_partner *cp, uval addr, uval size)
{
	union tce_bdesc bd;

	bd.lbd_bits.lbd_len = size;
	bd.lbd_bits.lbd_addr = addr;
	return tce_bd_xlate(&cp->cp_tce_data, bd);
}

static sval
crq_setup_osdata(struct crq_partner *cp, uval buffer, uval buffersize)
{
	cp->cp_flags = CRQ_FLAG_USED;

	cp->cp_flags |= CRQ_FLAG_HAS_PARTNER;
	cp->cp_partner->cp_flags |= CRQ_FLAG_HAS_PARTNER;

	cp->cp_num_entries = buffersize / sizeof (struct crq_entry);
	cp->cp_next_entry = 0;
	cp->cp_crqs = (struct crq_entry *)
		crq_addr_lookup(cp, buffer, buffersize);

	assert(cp->cp_tce_data.t_tce != NULL, "TCE was not set up!\n");

	DEBUG_OUT(DBG_CRQ, "%s: ---- crq buffer at %p  irq=0x%x\n",
		  __func__, cp->cp_crqs, cp->cp_interrupt);

	return 1;
}

/*
 * The following functions are directly called through the
 * h-calls.
 */
static sval
crq_tce_put(struct os *os, uval32 liobn, uval ioba, union tce ltce)
{
	sval ret;
	struct crq_partner *cp;

	cp = crq_get_crq(liobn);

	if (cp == NULL) {
		return H_Parameter;
	}

	if (os != crq_getos(cp)) {
		return H_Parameter;
	}

	assert(cp->cp_tce_data.t_tce != NULL,
	       "TCE was not set up (liobn=0x%x)!\n", liobn);

	ret = tce_put(os, &cp->cp_tce_data, ioba, ltce);

	return ret;
}

sval
crq_try_copy(struct cpu_thread *thread, uval32 sliobn, uval sioba,
	     uval32 dliobn, uval dioba, uval len)
{
	sval ret;
	struct os *os = thread->cpu->os;
	struct crq_partner *src,
	           *dst;
	uval *s_pa,
	    *d_pa;

	/*
	 * There must be a better way of doing this on PPC
	 * with a valid device tree.
	 */
	if ((dliobn & ~0U) == ~0U) {
		/* only for x86 */
		src = crq_get_crq(sliobn);
		if (src == NULL) {
			return H_Parameter;
		}
		dst = src->cp_partner;
	} else {
		/* destination: to IO partition */
		dst = crq_get_crq(dliobn);
		if (dst == NULL) {
			return H_Parameter;
		}
		src = dst->cp_partner;
	}

	if (os != crq_getos(src) && os != crq_getos(dst)) {
		return H_Parameter;
	}

	lock_acquire(src->cp_lock_ptr);

	d_pa = crq_addr_lookup(dst, dioba, len);
	if (d_pa) {
		s_pa = crq_addr_lookup(src, sioba, len);
		if (s_pa) {
			DEBUG_OUT(DBG_CRQ,
				  " %s: Copying from %p to %p\n",
				  __func__, s_pa, d_pa);
			copy_mem(d_pa, s_pa, len);
			ret = H_Success;
		} else {
			ret = H_Parameter;
		}

	} else {
		ret = H_Parameter;
	}

	lock_release(src->cp_lock_ptr);

	return ret;
}

sval
crq_free_crq(struct os *os, uval uaddr)
{
	sval ret;
	struct crq_partner *cp;

	cp = crq_get_crq(uaddr);

	DEBUG_OUT(DBG_CRQ, "%s: u_addr: 0x%lx\n", __func__, uaddr);

	if (cp == NULL) {
		return H_Parameter;
	}

	if (os != crq_getos(cp)) {
		return H_Parameter;
	}

	ret = crq_release_internal(cp);

	return ret;
}

sval
crq_reg(struct cpu_thread *thread, uval uaddr, uval queue, uval len)
{
	sval ret = H_Parameter;
	struct crq_partner *cp;
	struct os *os = thread->cpu->os;

	DEBUG_OUT(DBG_CRQ, "%s: uaddr: 0x%lx queue: 0x%lx len: 0x%lx\n",
		  __func__, uaddr, queue, len);

	if (len < CRQ_MIN_BUFFER_SIZE ||
	    0 != (len & (CRQ_MIN_BUFFER_SIZE - 1)) ||
	    queue != ALIGN_DOWN(queue, len)) {
		return H_Parameter;
	}

	cp = crq_get_crq(uaddr);

	if (cp == NULL) {
		return H_Parameter;
	}

	if (os != crq_getos(cp)) {
		return H_Parameter;
	}

	lock_acquire(cp->cp_lock_ptr);

	if (crq_getos(cp) == os) {
		if (crq_check_unused(cp)) {
			struct crq_partner *peer = cp->cp_partner;

			crq_setup_osdata(cp, queue, len);
			if (crq_check_unused(peer)) {
				ret = H_Closed;
			} else {
				ret = H_Success;
			}
		} else {
			ret = H_Resource;
		}
	}

	lock_release(cp->cp_lock_ptr);

	return ret;
}

sval
crq_send(struct os *os, uval uaddr, uval8 *msg)
{
	sval ret;
	struct crq_partner *cp;
	struct crq_partner *peer;

	cp = crq_get_crq(uaddr);
	if (cp == NULL) {
		return H_Parameter;
	}

	if (os != crq_getos(cp)) {
		return H_Parameter;
	}

	peer = cp->cp_partner;

	lock_acquire(peer->cp_lock_ptr);

	if (crq_haspartner(peer)) {
		ret = crq_put_entry_locked(peer, msg);
	} else {
		DEBUG_OUT(DBG_CRQ, "%s: Not sending this message since "
			  "CRQ has no partner.\n", __func__);

		ret = H_Parameter;
	}

	lock_release(peer->cp_lock_ptr);

	return ret;
}

static sval
crq_struct_init(struct crq_partner *cp, uval xenc,
		struct cpu_thread *thread, uval dma_range)
{
	struct os *os = thread->cpu->os;

	memset(cp, 0x0, sizeof (*cp));
	if (tce_alloc(&cp->cp_tce_data, 0x0, dma_range)) {
		struct sys_resource *res;
		struct vios_resource *vres;
		uval dum;
		sval rc;

		lock_init(&cp->cp_lock);

		/* I am the owner of this */
		cp->cp_os = os;
		cp->cp_interrupt = xenc;
		cp->cp_owner_res = 0;
		cp->cp_res[0].vr_liobn = xenc;
		cp->cp_res[1].vr_liobn = xenc;
		cp->cp_flags |= CRQ_FLAG_VALID;
		cp->cp_dma_size = dma_range;

		res = crq_getsres(cp);
		vres = crq_getvres(cp);
		resource_init(res, NULL, INTR_SRC);

		xir_default_config(cp->cp_interrupt, thread, cp);

		rc = insert_resource(res, os);
		assert(rc >= 0, "Can't give CRQ: 0x%lx to os\n",
		       vres->vr_liobn);

		lock_acquire(&res->sr_lock);
		rc = accept_locked_resource(res, &dum);
		assert(rc == H_Success, "Can't bind crq to os\n");
		lock_release(&res->sr_lock);

		return 1;
	}
	return 0;
}

static __inline__ void
crq_struct_clear(struct crq_partner *cp)
{
	tce_free(&cp->cp_tce_data);
	xir_default_config(cp->cp_interrupt, NULL, NULL);
}

static sval
crq_pair_setup(struct cpu_thread *thread,
	       struct crq_partner *cp1, uval xenc1,
	       struct crq_partner *cp2, uval xenc2, uval dma_range)
{
	if (crq_struct_init(cp1, xenc1, thread, dma_range)) {
		if (crq_struct_init(cp2, xenc2, thread, dma_range)) {
			/* Connect the two */
			cp1->cp_partner = cp2;
			cp2->cp_partner = cp1;

			/* use one common lock to avoid possible deadlocks */
			cp1->cp_lock_ptr = &cp1->cp_lock;
			cp2->cp_lock_ptr = &cp1->cp_lock;

			DEBUG_OUT(DBG_CRQ, "%s: Successfully set up TCEs for "
				  "CRQ on xenc1=0x%lx, xnec2=0x%lx.\n",
				  __func__, xenc1, xenc2);

			return 1;
		}
		crq_struct_clear(cp1);
	}

	return 0;
}

static sval
crq_acquire(struct cpu_thread *thread, uval dma_range)
{
	struct crq_partner *cp;
	sval isrc;
	uval xenc1,
	     xenc2;

	isrc = xir_find(XIRR_CLASS_CRQ);
	assert(isrc != -1, "Can't register crq!");

	if (isrc >= 0) {
		xenc1 = xirr_encode(isrc, XIRR_CLASS_CRQ);

		/* have to do find again (?) */
		isrc = xir_find(XIRR_CLASS_CRQ);
		assert(isrc != -1, "Can't register crq!");

		if (isrc >= 0) {
			xenc2 = xirr_encode(isrc, XIRR_CLASS_CRQ);

			DEBUG_OUT(DBG_CRQ,
				  "xenc1=0x%lx xenc2=0x%lx\n", xenc1, xenc2);

			cp = halloc(sizeof (struct crq_partner[2]));

			if (NULL != cp) {
				uval rc;

				rc = crq_pair_setup(thread,
						    &cp[0], xenc1,
						    &cp[1], xenc2, dma_range);

				if (rc) {
					return_arg(thread, 1, xenc1);
					return_arg(thread, 2, xenc2);
					return_arg(thread, 3, dma_range);

					return H_Success;
				}
			}
		}
	}

	DEBUG_OUT(DBG_CRQ, "%s: Could NOT set up TCEs for CRQ", __func__);

	return H_UNAVAIL;
}

static sval
crq_release(struct os *os, uval32 liobn)
{
	struct crq_partner *cp;

	cp = crq_get_crq(liobn);

	if (cp == NULL) {
		return H_Parameter;
	}

	if (crq_getos(cp) != os) {
		return H_Parameter;
	}

	if (!crq_release_internal(cp)) {
		return H_Busy;
	}

	return H_Success;
}

static sval
crq_accept(struct os *os, uval liobn, uval *retval)
{
	struct crq_partner *cp;

	cp = crq_get_crq(liobn);

	if (NULL != cp) {
		if (os == crq_getos(cp)) {
			*retval = liobn;
			return H_Success;
		}
	}

	return H_Parameter;
}

static sval
crq_invalidate(struct os *os, uval liobn)
{
	(void)os;
	struct crq_partner *cp;

	cp = crq_get_crq(liobn);

	if (cp == NULL) {
		return H_Parameter;
	}

	if (os == crq_getos(cp)) {
		cp->cp_flags &= ~CRQ_FLAG_VALID;
	}
	return H_Success;
}

static sval
crq_return(struct os *os, uval liobn)
{
	struct crq_partner *cp;

	cp = crq_get_crq(liobn);

	if (cp == NULL) {
		return H_Parameter;
	}
	if (os == crq_getos(cp)) {
		DEBUG_OUT(DBG_CRQ, "%s: owner\n", __func__);
	}
	return H_Success;
}

static sval
crq_grant(struct sys_resource **res,
	  struct os *src, struct os *dst, uval liobn)
{
	struct crq_partner *cp;
	struct cpu_thread *thread = &dst->cpu[0]->thread[0];
	uval old;
	uval new;

	cp = crq_get_crq(liobn);

	if (cp == NULL) {
		return H_Parameter;
	}

	if (crq_getos(cp) != src) {
		return H_Parameter;
	}

	/* set new owner */
	cp->cp_os = dst;
	/* where it's coming from */
	cp->cp_reaper = src;

	old = cp->cp_owner_res;
	new = old ^ 1;

	assert(cp->cp_res[new].vr_res.sr_owner == NULL, "new owner not 0\n");
	if (cp->cp_res[new].vr_res.sr_owner == NULL) {
		/* disable interrupts */
		xir_default_config(cp->cp_interrupt, NULL, NULL);

		cp->cp_owner_res = new;
		cp->cp_res[new].vr_res.sr_owner = dst;
		*res = &cp->cp_res[new].vr_res;

		resource_init(*res, NULL, INTR_SRC);

		tce_ia(&cp->cp_tce_data);
		force_rescind_resource(&cp->cp_res[old].vr_res);

		xir_default_config(cp->cp_interrupt, thread, cp);

		return H_Success;
	}

	return H_UNAVAIL;
}

static sval
crq_rescind(struct os *os, uval liobn)
{
	hprintf("Rescinding 0x%lx\n", liobn);
	struct crq_partner *cp;

	cp = crq_get_crq(liobn);

	if (cp == NULL) {
		return H_Parameter;
	}

	if (os == crq_getos(cp)) {
		uval other = cp->cp_owner_res ^ 1;

		if (cp->cp_res[other].vr_res.sr_owner != NULL) {
			force_rescind_resource(&cp->cp_res[other].vr_res);
		}
		return crq_release(os, liobn);
	}
	/* just allow the rescind to happen */
	return H_Success;
}

static sval
crq_conflict(struct sys_resource *res1, struct sys_resource *res2, uval liobn)
{
	struct crq_partner *cp;

	cp = crq_get_crq(liobn);
	if (cp == NULL) {
		return H_Parameter;
	}

	(void)res1;
	(void)res2;
	assert(0, "what do we do here?\n");

	return H_Success;
}

static struct vios crq_vios = {
	.vs_name = "crq",
	.vs_tce_put = crq_tce_put,
	.vs_acquire = crq_acquire,
	.vs_release = crq_release,
	.vs_grant = crq_grant,
	.vs_accept = crq_accept,
	.vs_invalidate = crq_invalidate,
	.vs_return = crq_return,
	.vs_rescind = crq_rescind,
	.vs_conflict = crq_conflict
};

sval
crq_sys_init(void)
{
	if (vio_register(&crq_vios, HVIO_CRQ) == HVIO_CRQ) {
		xir_init_class(XIRR_CLASS_CRQ, NULL, NULL);

		return 1;
	}
	debug_level = (1 << DBG_CRQ);
	assert(0, "failed to register: %s\n", crq_vios.vs_name);
	return 0;
}
