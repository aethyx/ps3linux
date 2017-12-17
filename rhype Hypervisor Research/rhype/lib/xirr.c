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
 *
 */
#include <lib.h>
#include <xirr.h>
#include <debug.h>

static struct xh_data xhd_external[XIRR_DEVID_SZ];
struct xh_data *xh_data = xhd_external;

static inline int xirr_check_range(struct xh_data *xhd, xirr_t xirr) {
	xhd = xhd;
	if (xirr_dev_id(xirr) >= XIRR_DEVID_SZ) {
		assert (0, "xirr: 0x%x out of range)\n", xirr);
		return 0;
	}
	return 1;
}

void *
xirr_get_data(struct xh_data *xhd, xirr_t xirr)
{
	struct xh_data *xh;

	if (!xirr_check_range(xhd, xirr)) {
		assert(0, "tried to clear and xirr that is out of range\n");
		return NULL;
	}
	xh = xirr_ptr(xhd, xirr);
	return xh->xh_data;
}

uval32
xirr_get_flags(struct xh_data *xhd, xirr_t xirr)
{
	struct xh_data *xh;

	if (!xirr_check_range(xhd, xirr)) {
		assert(0, "tried to clear and xirr that is out of range\n");
		return XIRR_INVALID;
	}
	xh = xirr_ptr(xhd, xirr);
	return xh->xh_flags;
}

void *
xirr_set_data(struct xh_data *xhd, xirr_t xirr, void *data)
{
	struct xh_data *xh;
	void *ret;

	if (!xirr_check_range(xhd, xirr)) {
		assert(0, "tried to clear and xirr that is out of range\n");
		return NULL;
	}
	xh = xirr_ptr(xhd, xirr);
	ret = xh->xh_data;

	xh->xh_data = data;
	return ret;
}


uval
xirr_set_flags(struct xh_data *xhd, xirr_t xirr, uval f)
{
	struct xh_data *xh;
	uval flags;

	if (!xirr_check_range(xhd, xirr)) {
		assert(0, "tried to clear and xirr that is out of range\n");
		return 0;
	}
	xh = xirr_ptr(xhd, xirr);
	flags = xh->xh_flags;
	xh->xh_flags = f;
	return flags;
}

int
xirr_set_handler(struct xh_data *xhd, xirr_t xirr,
		 xirr_func_t h, void *d)
{
	struct xh_data *xh;

	if (!xirr_check_range(xhd, xirr)) {
		return 0;
	}
	xh = xirr_ptr(xhd, xirr);

	__xirr_lock(xh);

	if (xh->xh_handler != NULL) {
		hprintf("xirr handler: 0x%x is already registered\n", xirr);
		__xirr_unlock(xh);
		return 0;
	}

	assert((h != NULL),
	       "must define a handler funtion\n");
	DEBUG_MT(DBG_INTRPT,"xirr: 0x%x handler:%p and with data @ %p\n",
		 xirr, h, d);

	xh->xh_data = d;
	xh->xh_handler = h;
	xh->xh_flags = __XIRR_LOCK_BIT | XIRR_INUSE;
	__xirr_unlock(xh);
	return 1;
}

void
xirr_clear_handler(struct xh_data *xhd, xirr_t xirr)
{
	struct xh_data *xh;

	if (!xirr_check_range(xhd, xirr)) {
		return;
	}
	xh = xirr_ptr(xhd, xirr);

	assert((xh->xh_handler != NULL),
	       "nothing to clear\n");

	DEBUG_MT(DBG_INTRPT,"xirr: 0x%x clearing handler\n", xirr);

	/* FIXME: need to dequeue this interrupt from the partition */
	__xirr_lock(xh);
	xh->xh_handler = NULL;
	xh->xh_data = NULL;
	xh->xh_flags = __XIRR_LOCK_BIT;
	__xirr_unlock(xh);
}

int
xirr_handle(struct xh_data *xhd, xirr_t xirr, uval payload,
	    struct cpu_thread** receiver)
{
	int ret = 0;
	struct xh_data *xh;

	assert(xirr_check_range(xhd, xirr),
	       "xirr: 0x%x is out of range for routing table\n", xirr);

	xh = xirr_ptr(xhd, xirr);
	__xirr_lock(xh);
	if (xh->xh_handler != NULL &&
	    (xh->xh_flags & XIRR_DISABLED)==0) {
		ret = xh->xh_handler(xirr, xh, payload, receiver);
	} else {
		xh->xh_flags |= XIRR_PENDING;
	}
	if (receiver) *receiver = NULL;

	__xirr_unlock(xh);
	return ret;
}


void
__xirr_lock(struct xh_data* xhd)
{
	lockbit(&xhd->xh_flags, __XIRR_LOCK_BIT);
}

void
xirr_lock(struct xh_data* xhd, xirr_t xirr)
{
	struct xh_data *xh;

	assert(xirr_check_range(xhd, xirr),
	       "xirr: 0x%x is out of range for routing table\n", xirr);

	xh = xirr_ptr(xhd, xirr);

	__xirr_lock(xh);

}

void
__xirr_unlock(struct xh_data* xhd)
{
	unlockbit(&xhd->xh_flags, __XIRR_LOCK_BIT);
}

void
xirr_unlock(struct xh_data* xhd, xirr_t xirr)
{
	struct xh_data *xh;

	assert(xirr_check_range(xhd, xirr),
	       "xirr: 0x%x is out of range for routing table\n", xirr);

	xh = xirr_ptr(xhd, xirr);

	__xirr_unlock(xh);
}
