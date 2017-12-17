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
 * This exists only for backward compatibility with VTERM in the PAPR
 */

#include <vterm.h>
#include <atomic.h>
#include <util.h>
#include <mmu.h>
#include <pgalloc.h>
#include <hype.h>
#include <tce.h>
#include <vio.h>
#include <objalloc.h>
#include <imemcpy.h>
#include <debug.h>

/* hack */
#include <vtty.h>

struct vt_unpaired {
	struct dlist vu_dlist;
	lock_t vu_lock;
};

static struct vt_unpaired vt_unpaired;

struct vt {
	struct vios_resource vt_res[2];
	uval vt_owner_res;
	uval vt_interrupt;
	uval vt_valid;
	uval vt_dma_size; /* is 0 then it must be client interface */
	struct dlist vt_dlist;
	struct vt *vt_partner;
	struct vt_fifo *vt_get; /* server side get */
	struct vt_fifo *vt_put; /* server side put */
};

struct vt_fifo {
	uval16 vtf_r;
	uval16 vtf_w;
	uval16 vtf_sz;
	uval8 vtf_reap;
	/* reject condition and interrupt delivery should be symetric */
	uval8 vtf_full;		/* Consider this to be reject and can
				 * only be turned off by a read. */
	lock_t vtf_lock;
	char vtf_b[0];
};

static uval
vtf_write(struct vt_fifo *f, uval c, const char buf[16])
{
	uval sz;

	if (f->vtf_reap) {
		return 0;
	}

	lock_acquire(&f->vtf_lock);

	if (f->vtf_w < f->vtf_r) {
		sz = f->vtf_r - f->vtf_w;
	} else {
		sz = f->vtf_sz - f->vtf_w;
		sz += f->vtf_r;		/* cover the wrap */
	}

	/* never catch up */
	--sz;

	if (sz >= c) {
		uval i;
		for (i = 0; i < c; i++) {
			f->vtf_b[f->vtf_w] = buf[i];
			++f->vtf_w;
			f->vtf_w %= f->vtf_sz;
		}
		f->vtf_full = 0;
	} else {
		c = 0;
		f->vtf_full = 1;
	}
	lock_release(&f->vtf_lock);

	return c;
}

static uval
vtf_read(struct vt_fifo *f, char *buf)
{
	uval sz;

	lock_acquire(&f->vtf_lock);

	if (f->vtf_r == f->vtf_w) {
		sz = 0;
	} else	if (f->vtf_r < f->vtf_w) {
		sz = f->vtf_w - f->vtf_r;
	} else {
		sz = f->vtf_sz - f->vtf_r;
		sz += f->vtf_w;		/* cover the wrap */
	}
	/* read can catch up with write */
	sz = MIN(sz, 16);
	if (sz > 0) {
		uval i;
		for (i = 0; i < sz; i++) {
			buf[i] = f->vtf_b[f->vtf_r];
			++f->vtf_r;
			f->vtf_r %= f->vtf_sz;
		}
		/* Lets reset the cursors to keep using cahce line */
		if (f->vtf_r == f->vtf_w) {
			f->vtf_r = 0;
			f->vtf_w = 0;
		}

		f->vtf_full = 0;
	}
	lock_release(&f->vtf_lock);

	return sz;
}

static uval
vtf_alloc(struct vt_fifo *f[2], uval sz)
{
	uval p;
	uval half;
	uval fsz;

	sz = PGSIZE;

	half = (sz / 2);
	fsz = half - sizeof (struct vt_fifo);

	p = (uval)halloc(sz);
	if (p == PAGE_ALLOC_ERROR) {
		return 0;
	}

	memset((void *)p, 0, sz);

	f[0] = (struct vt_fifo *)p;
	f[0]->vtf_sz = fsz;
	lock_init(&f[0]->vtf_lock);

	f[1] = (struct vt_fifo *)(p + half);
	f[1]->vtf_sz = fsz;
	lock_init(&f[1]->vtf_lock);

	return sz;
}

static uval
vtf_free(struct vt_fifo *f[2], uval sz)
{
	if ((f[0]->vtf_r != f[0]->vtf_w)) {
		/* still stuff left for the server to read */
		return 0;
	}
	free_pages(&phys_pa, (uval)f[0], sz);
	f[0] = NULL;
	f[1] = NULL;

	return 1;
}

static __inline__ uval
vt_isserver(struct vt *vt)
{
	return (vt->vt_dma_size > 0);
}


static __inline__ struct vios_resource *
vt_getvres(struct vt *vt)
{
	return &vt->vt_res[vt->vt_owner_res];
}

static __inline__ struct sys_resource *
vt_getsres(struct vt *vt)
{
	struct vios_resource *res;

	res = vt_getvres(vt);

	return &res->vr_res;
}

static __inline__ struct os *
vt_getos(struct vt *vt)
{
	struct vios_resource *res;

	res = vt_getvres(vt);

	return res->vr_res.sr_owner;
}

static inline struct vt *
vt_get(uval32 liobn)
{
	return (struct vt *)xir_get_device(liobn);
}

sval
vt_char_put(struct cpu_thread *thread, uval chan, uval c, const char buf[16])
{
	struct os *os = thread->cpu->os;
	struct vt *vt;
	uval sz;

	if (os == ctrl_os) {
		if (chan < CHANNELS_PER_OS) {
			/* punt for now */
			return vtty_put_term_char16(thread, chan, c, buf);
		}
	}

	if (chan < 2) {
		if (os->po_vt[chan] == 0) {
			return vtty_put_term_char16(thread, chan, c, buf);
		}
		chan = os->po_vt[chan];
	}

	vt = vt_get(chan);
	if (vt == NULL) {
		return  H_Parameter;
	}

	if (vt_getos(vt) != os) {
		return H_Parameter;
	}

	if (vt->vt_partner == NULL) {
		return H_Closed;
	}

	sz = vtf_write(vt->vt_put, c, buf);
	if (sz == 0) {
		return H_Busy;
	}
	xir_raise(vt->vt_partner->vt_interrupt, NULL);
	return H_Success;
}

sval
vt_char_get(struct cpu_thread *thread, uval chan, char *buf)
{
	struct os *os = thread->cpu->os;
	struct vt *vt;
	uval signal;
	uval sz;

	if (os == ctrl_os) {
		if (chan < CHANNELS_PER_OS) {
			/* punt for now */
			return vtty_get_term_char16(thread, chan, buf);
		}
	}

	if (chan < 2) {
		if (os->po_vt[chan] == 0) {
			return vtty_get_term_char16(thread, chan, buf);
		}
		chan = os->po_vt[chan];
	}

	vt = vt_get(chan);
	if (vt == NULL) {
		return  H_Parameter;
	}

	if (vt_getos(vt) != os) {
		return H_Parameter;
	}

	/* this allows for the server to drain */
	if (vt->vt_partner == NULL) {
		return H_Closed;
	}

	signal = vt->vt_get->vtf_full;

	sz = vtf_read(vt->vt_get, buf);

	if (sz > 0) {
		if (signal) {
			/* notify partner that there is space where there was
			 * none before */
			xir_raise(vt->vt_partner->vt_interrupt, NULL);
		}
	}

	return_arg(thread, 1, sz);

	return H_Success;
}

static struct vt *
vt_acquire_internal(void)
{
	struct vt *vt;

	vt = halloc(sizeof (*vt));
	if (vt) {
		sval isrc;
		uval xenc;

		isrc = xir_find(XIRR_CLASS_VTERM);
		assert(isrc != -1, "Can't get xirr for vterm\n");
		xenc = xirr_encode(isrc, XIRR_CLASS_VTERM);

		if (isrc >= 0) {
			memset(vt, 0, sizeof (*vt));

			vt->vt_owner_res = 0;
			vt->vt_valid = 1;
			vt->vt_res[0].vr_liobn = xenc;
			vt->vt_res[1].vr_liobn = xenc;
			vt->vt_interrupt = xenc;
			dlist_init(&vt->vt_dlist);

			struct sys_resource *res = vt_getsres(vt);
			resource_init(res, NULL, INTR_SRC);

			return vt;
		}
		xir_default_config(xenc, NULL, NULL);
	}
	hfree(vt, sizeof(*vt));
	return NULL;
}

static sval
vts_free_internal(struct vt *vt)
{
	struct vt *pvt;

	pvt = vt->vt_partner;

	if (pvt != NULL) {
		/* fixme: locks? */
		pvt->vt_put = NULL;
		pvt->vt_get = NULL;
		pvt->vt_partner = NULL;

		/* put partner back in list */
		lock_acquire(&vt_unpaired.vu_lock);
		dlist_insert(&pvt->vt_dlist, &vt_unpaired.vu_dlist);
		lock_release(&vt_unpaired.vu_lock);
	}

	vt->vt_partner = NULL;
	if (vt->vt_get->vtf_r != vt->vt_get->vtf_w) {
		hprintf("%s: stuff left for the server to read: Busy?\n",
			__func__);
	}
	if (vt->vt_put->vtf_r != vt->vt_put->vtf_w) {
		hprintf("%s: stuff left for the client to read: Busy?\n",
			__func__);
	}

	vt->vt_get->vtf_r = 0;
	vt->vt_get->vtf_w = 0;
	vt->vt_put->vtf_r = 0;
	vt->vt_put->vtf_w = 0;

	return H_Success;
}

static uval
vt_release_internal(struct vt *vt)
{
	struct vt *pvt;

	pvt = vt->vt_partner;

	DEBUG_OUT(DBG_VTERM, "%s: 0x%lx\n", __func__, vt->vt_interrupt);

	if (vt_isserver(vt)) {
		struct vt_fifo *f[2];

		vts_free_internal(vt);

		f[0] = vt->vt_get;
		f[1] = vt->vt_put;

		if (!vtf_free(f, vt->vt_dma_size)) {
			return H_Busy;
		}
	} else {
		if (pvt != NULL) {
			/* raise interrupt so we'll drain and see busy */
			xir_raise(pvt->vt_interrupt, NULL);
			pvt->vt_partner = NULL;
		}
	}
	xir_default_config(vt->vt_interrupt, NULL, NULL);
	hfree(vt, sizeof(*vt));

	return 1;
}

static uval
vt_res_init(struct cpu_thread *thread, struct vt *vt)
{
	struct vios_resource *vres;
	struct sys_resource *res;
	uval dum;
	sval rc;

	vres = vt_getvres(vt);
	res = vt_getsres(vt);
	xir_default_config(vt->vt_interrupt, thread, vt);
	rc = insert_resource(res, thread->cpu->os);
	assert(rc >= 0, "Can't give vterm: 0x%lx to os\n",
	       vres->vr_liobn);

	lock_acquire(&res->sr_lock);
	rc = accept_locked_resource(res, &dum);
	assert(rc == H_Success, "Can't bind vterm to os\n");
	lock_release(&res->sr_lock);

	return 1;
}

static sval
vts_acquire(struct cpu_thread *thread, uval dma_range)
{
	struct vt *vts;

	vts = vt_acquire_internal();
	if (vts) {
		struct vt_fifo *f[2];

		dma_range = vtf_alloc(f, dma_range);
		if (dma_range > 0) {
			/* server end is in this order */
			vts->vt_get = f[0];
			vts->vt_put = f[1];
			vts->vt_dma_size = dma_range;
			vt_res_init(thread, vts);

			return_arg(thread, 1,
				   vts->vt_res[0].vr_liobn);
			return_arg(thread, 2, 0);
			return_arg(thread, 3, dma_range);

			return H_Success;
		}
	}
	vt_release_internal(vts);
	return H_UNAVAIL;

}

static sval
vtc_acquire(struct cpu_thread *thread)
{
	struct vt *vtc;

	vtc = vt_acquire_internal();
	if (vtc) {
		vt_res_init(thread, vtc);

		vtc->vt_dma_size = 0;

		/* add to unpaired list */
		lock_acquire(&vt_unpaired.vu_lock);
		dlist_insert(&vtc->vt_dlist, &vt_unpaired.vu_dlist);
		lock_release(&vt_unpaired.vu_lock);

		return_arg(thread, 1, vtc->vt_res[0].vr_liobn);
		return_arg(thread, 2, 0);
		return_arg(thread, 3, 0);
		return H_Success;
	}
	vt_release_internal(vtc);
	return H_UNAVAIL;
}

static sval
vt_acquire(struct cpu_thread *thread, uval dma_range)
{
	if (dma_range == 0) {
		return vtc_acquire(thread);
	}
	return vts_acquire(thread, dma_range);
}

static sval
vt_release(struct os *os, uval32 liobn)
{
	struct vt *vt;

	vt = vt_get(liobn);

	if (vt == NULL) {
		return H_Parameter;
	}
	if (vt_getos(vt) != os) {
		return H_Parameter;
	}

	if (!vt_release_internal(vt)) {
		return H_Busy;
	}
	return H_Success;
}

sval
vts_register(struct os *os, uval ua, uval lpid, uval pua)
{
	struct vt *vt;
	struct vt *pvt;
	struct os *pos;


	DEBUG_OUT(DBG_VTERM, "%s: enter: ua: 0x%lx, lpid: 0x%lx, pua: 0x%lx\n",
		__func__, ua, lpid, pua);

	vt = vt_get(ua);
	if (vt == NULL) {
		return H_Parameter;
	}

	if (vt_getos(vt) != os) {
		return H_Parameter;
	}

	pos = os_lookup(lpid);
	if (pos == NULL) {
		return H_Parameter;
	}
	if (!vt_isserver(vt)) {
		/* vt is not a server */
		return H_Parameter;
	}

	pvt = vt_get(pua);
	if (pvt == NULL) {
		return H_Parameter;
	}
	if (vt_getos(pvt) != pos) {
		return H_Parameter;
	}

	if (vt_isserver(pvt)) {
		/* pvt is not a client */
		return H_Parameter;
	}

	if (!vt->vt_valid) {
		return H_Parameter;
	}

	if (cas_ptr(&vt->vt_partner, NULL, pvt)) {
		if (cas_ptr(&pvt->vt_partner, NULL, vt)) {
			pvt->vt_put = vt->vt_get;
			pvt->vt_get = vt->vt_put;

			/* take off unpaired list */
			lock_acquire(&vt_unpaired.vu_lock);
			dlist_detach(&pvt->vt_dlist);
			lock_release(&vt_unpaired.vu_lock);

			DEBUG_OUT(DBG_VTERM, "%s: success\n", __func__);

			return H_Success;
		}
		vt->vt_partner = NULL;
	}

	return H_Parameter;
}

sval
vts_free(struct os *os, uval ua)
{
	struct vt *vt;

	if (os != ctrl_os) {
		return H_Parameter;
	}
	if (ua < CHANNELS_PER_OS) {
		return  H_Parameter;
	}
	vt = vt_get(ua);
	if (vt == NULL) {
		return H_Parameter;
	}
	return vts_free_internal(vt);
}

sval
vts_partner_info(struct os *os, uval ua, uval plpid, uval pua, uval lba)
{
	struct vt *vt;
	struct dlist *curr;
	uval pba;
	uval sz;
	uval b[2 + 10]; /* 80 characters for strings */


	DEBUG_OUT(DBG_VTERM, "%s: enter: ua: 0x%lx, lpid: 0x%lx, pua: 0x%lx\n",
		  __func__, ua, plpid, pua);

	if (ua < CHANNELS_PER_OS) {
		return  H_Parameter;
	}
	vt = vt_get(ua);
	if (vt == NULL) {
		return H_Parameter;
	}
	if (!vt_isserver(vt)) {
		return H_Parameter;
	}
	if ((lba & (PGSIZE - 1)) != 0) {
		return H_Parameter;
	}

	pba = logical_to_physical_address(os, lba, PGSIZE);
	if (pba == INVALID_PHYSICAL_ADDRESS) {
		return H_Parameter;
	}

	lock_acquire(&vt_unpaired.vu_lock);
	curr = dlist_next(&vt_unpaired.vu_dlist);

	if (plpid != ~0UL && pua != ~0UL) {
		while (curr != &vt_unpaired.vu_dlist) {
			struct vt *pvt;
			struct os *pos;

			pvt = PARENT_OBJ(struct vt, vt_dlist, curr);
			pos = vt_getos(pvt);

			if (pos->po_lpid == plpid &&
			    pvt->vt_interrupt == pua) {
				break;
			}
			curr = dlist_next(curr);
		}
		if (curr == &vt_unpaired.vu_dlist) {
			lock_release(&vt_unpaired.vu_lock);
			return H_Parameter;
		}
		curr = dlist_next(curr);
	}

	/* we only report one at a time !? */
	if (curr !=  &vt_unpaired.vu_dlist) {
		struct vt *pvt;
		struct os *pos;
		uval len;
		char  *c;

		pvt = PARENT_OBJ(struct vt, vt_dlist, curr);

		pos = vt_getos(pvt);
		b[0] = pos->po_lpid;
		b[1] = pvt->vt_interrupt;

		c = (char *)&b[2];
		/* something to take us to a nice even number */
		sz = (sizeof b) - (sizeof(b[0]) * 2);
		len = snprintf(c, sz, "vterms suck 0x%lx", b[1]);
		c[len] = '\0';
		++len;
		sz = len + (sizeof(b[0]) * 2);
	} else {
		/* we reached the end */
		b[0] = ~0UL;
		b[1] = ~0UL;
		b[2] = 0UL;		/* Null string */
		sz = (sizeof(b[0]) * 3);
	}
	lock_release(&vt_unpaired.vu_lock);


		DEBUG_OUT(DBG_VTERM, "%s: success: 0x%lx, 0x%lx\n",
			  __func__, b[0], b[1]);

	/* one last check */
	pba = logical_to_physical_address(os, lba, PGSIZE);
	if (pba == INVALID_PHYSICAL_ADDRESS) {
		return H_Parameter;
	}
	copy_out((void *)pba, b, sz);
	return H_Success;
}

static sval
vt_tce_put(struct os *os __attribute__ ((unused)),
	   uval32 liobn __attribute__ ((unused)),
	   uval ioba __attribute__ ((unused)),
	   union tce ltce __attribute__ ((unused)))
{
	/* no TCEs related */
	return H_Parameter;
}

static sval
vt_grant(struct sys_resource **res,
	   struct os *src, struct os *dst, uval liobn)
{
	struct vt *vt;
	struct cpu_thread *thread = &dst->cpu[0]->thread[0];
	uval old;
	uval new;

	vt = vt_get(liobn);

	if (vt == NULL) {
		return H_Parameter;
	}

	assert(vt_getos(vt) == src, "check failed\n");

	old = vt->vt_owner_res;
	new = old ^ 1; /* flip between 0 and 1 */

	assert(vt->vt_res[new].vr_res.sr_owner == NULL, "new owner not 0\n");
	if (vt->vt_res[new].vr_res.sr_owner == NULL) {

		/* disable interrupts */
		xir_default_config(vt->vt_interrupt, NULL, NULL);

		vt->vt_owner_res = new;
		vt->vt_res[new].vr_res.sr_owner = dst;
		*res = &vt->vt_res[new].vr_res;

		resource_init(*res, NULL, INTR_SRC);

		force_rescind_resource(&vt->vt_res[old].vr_res);

		xir_default_config(vt->vt_interrupt, thread, vt);

		/* set things up so channal 0 works */
		if (!vt_isserver(vt)) {
			uval ch = ARRAY_SIZE(dst->po_vt);

			if (dst->po_vt[0] == 0) {
				ch = 0;
			} else if (dst->po_vt[1] == 0) {
				ch = 1;
			}
			if (ch < ARRAY_SIZE(dst->po_vt)) {
				dst->po_vt[ch] = vt->vt_res[0].vr_liobn;

				DEBUG_OUT(DBG_VTERM,
					  "seting up chan %ld as: 0x%lx\n",
					  ch, vt->vt_res[0].vr_liobn);

			}
		}

		return H_Success;
	}
	return H_UNAVAIL;
}

static sval
vt_accept(struct os *os, uval liobn, uval *retval)
{
	struct vt *vt;

	vt = vt_get(liobn);

	if (vt != NULL) {
		if (os == vt_getos(vt)) {
			*retval = liobn;
			return H_Success;
		}
	}
	return H_Parameter;
}

static sval
vt_invalidate(struct os *os, uval liobn)
{
	struct vt *vt;

	vt = vt_get(liobn);

	if (vt == NULL) {
		return H_Parameter;
	}
	if (os == vt_getos(vt)) {
		vt->vt_valid = 0;
	}
	return H_Success;
}
static sval
vt_return (struct os *os, uval liobn)
{
	struct vt *vt;

	vt = vt_get(liobn);

	if (vt == NULL) {
		return H_Parameter;
	}
	if (os == vt_getos(vt)) {
		DEBUG_OUT(DBG_VTERM, "%s: owner\n", __func__);
	}
	return H_Success;
}

static sval
vt_rescind(struct os *os, uval liobn)
{
	struct vt *vt;

	vt = vt_get(liobn);

	if (vt == NULL) {
		return H_Parameter;
	}

	if (os == vt_getos(vt)) {
		uval other = vt->vt_owner_res ^ 1;

		if (vt->vt_res[other].vr_res.sr_owner != NULL) {
			force_rescind_resource(&vt->vt_res[other].vr_res);
		}
		if (os->po_vt[0] == liobn) {
			os->po_vt[0] = 0;
		} else if (os->po_vt[1] == liobn) {
			os->po_vt[1] = 0;
		}
		return vt_release(os, liobn);
	}
	/* just allow the rescind to happen */
	return H_Success;
}

static sval
vt_conflict(struct sys_resource *res1,
	    struct sys_resource *res2,
	    uval liobn)
{
	struct vt *vt;

	vt = vt_get(liobn);

	if (vt == NULL) {
		return H_Parameter;
	}
	(void)res1; (void)res2;
	assert(0, "what do we do here?\n");

	return H_Success;
}


static struct vios vt_vios = {
	.vs_name = "vterm",
	.vs_tce_put = vt_tce_put,
	.vs_acquire = vt_acquire,
	.vs_release = vt_release,
	.vs_grant = vt_grant,
	.vs_accept = vt_accept,
	.vs_invalidate = vt_invalidate,
	.vs_return = vt_return,
	.vs_rescind = vt_rescind,
	.vs_conflict = vt_conflict
};

uval
vt_sys_init(void)
{
	if (vio_register(&vt_vios, HVIO_VTERM) == HVIO_VTERM) {
		xir_init_class(XIRR_CLASS_VTERM, NULL, NULL);
		dlist_init(&vt_unpaired.vu_dlist);
		lock_init(&vt_unpaired.vu_lock);

		return 1;
	}
	assert(0, "failed to register: %s\n", vt_vios.vs_name);
	return 0;
}

