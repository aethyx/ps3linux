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

#include <config.h>
#include <lpar.h>
#include <vio.h>
#include <llan.h>
#include <crq.h>
#include <os.h>
#include <io_xlate.h>
#include <xirr.h>

static struct vios *vios[16] = { NULL,};

static inline struct vios *
vios_get(uval i)
{
	if (i < ARRAY_SIZE(vios)) {
		return vios[i];
	}
	return NULL;
}

sval
vio_register(struct vios *v, uval t)
{
	if (t < ARRAY_SIZE(vios)) {
		if (vios[t] == NULL) {
			vios[t] = v;
			return t;
		}
	}
	return -1;
}

static sval
vio_acquire(struct cpu_thread *thread, uval type, uval dma_range)
{
	struct vios *v;

	v = vios_get(type);

	if (v == NULL) {
		assert(0, "bad liobn: 0x%lx\n", type);
		return H_Parameter;
	}
	return v->vs_acquire(thread, dma_range);
}


static sval
vio_release(struct cpu_thread *thread, uval32 liobn)
{
	uval t;
	struct vios *v;
	struct os *os = thread->cpu->os;

	t = vio_type(liobn);
	v = vios_get(t);

	if (v == NULL) {
		assert(0, "bad liobn: 0x%x\n", liobn);
		return H_Parameter;
	}
	return v->vs_release(os, liobn);
}

sval
vio_ctl(struct cpu_thread *thread, uval cmd, uval arg1, uval arg2)
{
	switch (cmd) {
	default:
		return H_Parameter;
	case HVIO_ACQUIRE:
		return vio_acquire(thread, arg1, arg2);
	case HVIO_RELEASE:
		return vio_release(thread, arg1);
	}
	return H_Function;
}

sval
vio_signal(struct os *os, uval uaddr, uval mode)
{
	uval t;
	struct vios *v;

	switch (mode) {
	case 0:
	case 1:
		break;
	default:
		return H_Parameter;
		break;
	}

	t = vio_type(uaddr);
	v = vios_get(t);

	if (v == NULL) {
		assert(0, "bad liobn: 0x%lx\n", uaddr);
		return H_Parameter;
	}

	if (v->vs_signal) {
		return v->vs_signal(os, uaddr, mode);
	}

	sval ret = H_Success;

	struct xh_data *xd = xir_get_xh_data(uaddr);
	struct cpu_thread *thr = xir_get_thread(uaddr);

	if (thr && thr->cpu->os == os) {
		xir_set_state(uaddr, xd, mode);
	} else {
		ret = H_Permission;
	}

	return ret;

}

sval
vio_tce_put(struct os *os, uval32 liobn, uval ioba, union tce tce)
{
	uval t;
	struct vios *v;

	t = vio_type(liobn);
	v = vios_get(t);

	if (v == NULL) {
		assert(0, "bad liobn: 0x%x\n", liobn);
		return H_Parameter;
	}
	if (v->vs_tce_put == NULL) {
		return H_Parameter; /* FIXME: Is this right? */
	}

	return v->vs_tce_put(os, liobn, ioba, tce);
}

static struct vios hw_vios = {
	.vs_name = "HW",
	.vs_tce_put = io_xlate_put,
	.vs_signal = NULL,
	.vs_acquire = NULL,
	.vs_release = NULL
};

void
vio_init(void)
{
	uval t = vio_register(&hw_vios, HVIO_HW);
	assert(t == HVIO_HW, "vio registration failed\n");
}

static sval
vio_grant(struct sys_resource **res, struct os *src,
	  uval flags __attribute__ ((unused)),
	  uval logical_hi,
	  uval logical_lo __attribute__ ((unused)),
	  uval size __attribute__ ((unused)),
	  struct os *dest)
{
	
	uval liobn = logical_hi;
	uval t;
	struct vios *v;

	t = vio_type(liobn);
	v = vios_get(t);

	if (v == NULL) {
		assert(0, "bad liobn: 0x%lx\n", liobn);
		return H_Parameter;
	}
	if (v->vs_grant == NULL) {
		return H_Parameter;
	}

	return v->vs_grant(res, src, dest, liobn);
}

static sval
vio_accept(struct sys_resource *sr, uval *retval)
{
	uval liobn;
	uval t;
	struct vios *v;
	struct vios_resource *r;

	r = PARENT_OBJ(struct vios_resource, vr_res, sr);

	liobn = r->vr_liobn;
	t = vio_type(liobn);
	v = vios_get(t);

	if (v == NULL) {
		assert(0, "bad liobn: 0x%lx\n", liobn);
		return H_Parameter;
	}
	if (v->vs_accept) {
		return v->vs_accept(sr->sr_owner, liobn, retval);
	}
	return H_Parameter;
}

static sval
vio_invalidate(struct sys_resource *sr)
{
	uval liobn;
	uval t;
	struct vios *v;
	struct vios_resource *r;

	r = PARENT_OBJ(struct vios_resource, vr_res, sr);

	liobn = r->vr_liobn;
	t = vio_type(liobn);
	v = vios_get(t);

	if (v == NULL) {
		assert(0, "bad liobn: 0x%lx\n", liobn);
		return H_Parameter;
	}
	if (v->vs_invalidate) {
		return v->vs_invalidate(sr->sr_owner, liobn);
	}
	return H_Parameter;
}

static sval
vio_return(struct sys_resource *res)
{
	uval liobn;
	uval t;
	struct vios *v;
	struct vios_resource *r;

	r = PARENT_OBJ(struct vios_resource, vr_res, res);

	liobn = r->vr_liobn;
	t = vio_type(liobn);
	v = vios_get(t);

	if (v == NULL) {
		assert(0, "bad liobn: 0x%lx\n", liobn);
		return H_Parameter;
	}
	if (v->vs_return) {
		return v->vs_return(res->sr_owner, liobn);
	}
	return H_Parameter;
}

static sval
vio_rescind(struct sys_resource *res)
{
	uval liobn;
	uval t;
	struct vios *v;
	struct vios_resource *r;

	r = PARENT_OBJ(struct vios_resource, vr_res, res);

	liobn = r->vr_liobn;
	t = vio_type(liobn);
	v = vios_get(t);

	if (v == NULL) {
		assert(0, "bad liobn: 0x%lx\n", liobn);
		return H_Parameter;
	}
	if (v->vs_rescind) {
		return v->vs_rescind(res->sr_owner, liobn);
	}
	return H_Parameter;
}

static uval
vio_conflict(struct sys_resource * res1, struct sys_resource * res2)
{
	uval liobn;
	uval t;
	struct vios *v;
	struct vios_resource *r;

	r = PARENT_OBJ(struct vios_resource, vr_res, res1);

	liobn = r->vr_liobn;
	t = vio_type(liobn);
	v = vios_get(t);

	if (v == NULL) {
		assert(0, "bad liobn: 0x%lx\n", liobn);
		return H_Parameter;
	}
	if (v->vs_conflict) {
		return v->vs_conflict(res1, res2, liobn);
	}
	return H_Parameter;
}

struct resource_action vio_actions = {
	.grant_fn = vio_grant,
	.accept_fn = vio_accept,
	.return_fn = vio_return,
	.rescind_fn = vio_rescind,
	.invalidate_fn = vio_invalidate,
	.conflict_fn = vio_conflict,
};
