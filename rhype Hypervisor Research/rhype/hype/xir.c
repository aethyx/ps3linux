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
#include <xir.h>
#include <arch_xir.h>
#include <hype.h>
#include <os.h>
#include <objalloc.h>

struct xirr_class_s xirr_classes[XIRR_CLASS_MAX];

int
xir_find(uval class)
{
	int x = 0;
	struct xirr_class_s *c = &xirr_classes[class];

	for (; x < XIRR_DEVID_SZ; ++x) {
		if (cas_uval32(&c->xc_data[x].xh_flags, 0, XIRR_INUSE)) {
			return x;
		}
	}
	return -1;
}

int
xir_grab(xirr_t xirr)
{
	struct xirr_class_s *c = &xirr_classes[xirr_class(xirr)];
	uval isrc = xirr_dev_id(xirr);
	if (cas_uval32(&c->xc_data[isrc].xh_flags, 0, XIRR_INUSE)) {
		return 1;
	}
	return 0;
}

int
__xir_enqueue(xirr_t xirr, struct xh_data *xhd, struct cpu_thread *thr)
{
	int ret = 0;
	uval flags = xhd->xh_flags;

	if (flags & XIRR_OUTSTANDING) {
		return 0;
	}

	flags |= XIRR_OUTSTANDING;
	flags &= ~XIRR_PENDING;
	xhd->xh_flags = flags;

	ret = xir_add(&thr->xirq, xirr);

	if (ret >= 0) {
		arch_xir_raise(thr);
		run_partition(thr);
	}
	return ret;
}

static int
__xir_raise(xirr_t xirr, struct xh_data *xh, uval payload,
	    struct cpu_thread **thread)
{
	(void)payload;
	int ret = 0;
	struct cpu_thread *thr = (struct cpu_thread *)xh->xh_data;

	ret = __xir_enqueue(xirr, xh, thr);
	if (ret >= 0 && thread) {
		*thread = thr;
	}
	return ret;
}

void
xir_set_state(xirr_t xirr, struct xh_data* xd, uval state)
{
	__xirr_lock(xd);

	uval flags = xd->xh_flags;

	if (state) {
		if ((flags & XIRR_DISABLED) && (flags & XIRR_PENDING)) {
			flags &= ~XIRR_DISABLED;

			xd->xh_flags = flags;
			__xirr_unlock(xd);

			xir_raise(xirr, NULL);
		} else {
			flags &= ~XIRR_DISABLED;
			xd->xh_flags = flags;
			__xirr_unlock(xd);
		}
	} else {
		flags |= XIRR_DISABLED;
		xd->xh_flags = flags;
		__xirr_unlock(xd);

	}

}

int
xir_inject(uval class, uval dev_id, uval payload, struct cpu_thread **thread)
{
	int ret = XH_DROP;
	struct xh_data *xd = __xir_get_xh_data(class, dev_id);
	struct cpu_thread *thr;
	struct cpu_thread **pthr;

	if (thread) {
		pthr = thread;
	} else {
		pthr = &thr;
	}
	*pthr = NULL;

	__xirr_lock(xd);

	uval flags = xd->xh_flags;

	if (flags & XIRR_DISABLED) {
		flags |= XIRR_PENDING;
		xd->xh_flags = flags;
	} else if (xd->xh_handler) {
		ret = xd->xh_handler(xirr_encode(dev_id, class), xd,
				     payload, pthr);
	}
	__xirr_unlock(xd);
	return ret;
}

void
xir_eoi(struct cpu_thread *thread, xirr_t xirr)
{
	uval class = xirr_class(xirr);
	struct xh_data *xd = xirr_classes[class].xc_data;

	xirr_lock(xd, xirr);

	uval flags = xirr_get_flags(xd, xirr);

	flags &= ~XIRR_OUTSTANDING;

	xir_set_flags(xirr, flags);
	xirr_unlock(xd, xirr);

	arch_xir_eoi(thread);
}
void *
xir_get_device(xirr_t xirr)
{
	struct xh_data *xh = xir_get_xh_data(xirr);
	if (xh != NULL) {
		return xh->xh_device;
	}
	return NULL;
}

sval
xir_default_config(xirr_t xirr, struct cpu_thread *dest, void *device)
{
	struct xh_data *xh = xir_get_xh_data(xirr);
	sval ret = H_Success;

	if (xh == NULL) {
		return H_Parameter;
	}
	__xirr_lock(xh);

	if (dest == NULL) {
		xh->xh_flags &= __XIRR_LOCK_BIT;
		xh->xh_data = NULL;
		xh->xh_handler = NULL;
		xh->xh_device = device;
	} else if (xh->xh_handler) {
		ret = H_UNAVAIL;
	} else {
		xh->xh_flags &= __XIRR_LOCK_BIT;
		xh->xh_flags |= XIRR_INUSE | XIRR_DISABLED | XIRR_DATA_THR;
		xh->xh_handler = __xir_raise;
		xh->xh_data = dest;
		xh->xh_device = device;
	}
	__xirr_unlock(xh);

	return ret;
}

void
xir_init_class(int class, xirr_func_t handler, void *arg)
{
	struct xh_data *array;

	lock_init(&xirr_classes[class].xc_lock);

	array = halloc(sizeof (struct xh_data) * XIRR_DEVID_SZ);
	memset(array, 0, sizeof (struct xh_data) * XIRR_DEVID_SZ);

	xirr_classes[class].xc_data = array;

	if (handler) {
		int x = 0;

		for (; x < XIRR_DEVID_SZ; ++x, ++array) {
			array->xh_handler = handler;
			array->xh_data = arg;
			array->xh_flags = XIRR_INUSE;
		}
	}
}
