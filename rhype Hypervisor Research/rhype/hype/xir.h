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

#ifndef _XIR_H
#define _XIR_H
#include <lib.h>
#include <xirr.h>
#include <atomic.h>

#define XH_UNKNOWN -1
#define XH_DROP -2

#define XIRR_CLASS_MAX 15

struct xir_queue {
	uval16 xq_head;
	uval16 xq_tail;
	xirr_t xq_list[XIRR_DEVID_SZ];
};

struct xirr_class_s {
	lock_t xc_lock;
	struct xh_data *xc_data;
	sval (*eoi_fn) (struct cpu_thread * thr, xirr_t intr);
};

extern struct xirr_class_s xirr_classes[XIRR_CLASS_MAX];

static inline struct xh_data *
__xir_get_xh_data(uval class, uval dev_id)
{
	if (!xirr_classes[class].xc_data) {
		return NULL;
	}

	if (dev_id >= XIRR_DEVID_SZ) {
		return NULL;
	}

	return &xirr_classes[class].xc_data[dev_id];
}

static __attribute__ ((always_inline)) struct xh_data *
xir_get_xh_data(xirr_t xirr)
{
	uval class = xirr_class(xirr);
	uval dev_id = xirr_xisr(xirr);

	return __xir_get_xh_data(class, dev_id);
}

static inline struct cpu_thread *
__xir_get_thread(struct xh_data *xhd)
{
	if (xhd->xh_flags & XIRR_DATA_THR) {
		return (struct cpu_thread *) xhd->xh_data;
	}
	return NULL;
}

static inline struct cpu_thread *
xir_get_thread(xirr_t xirr)
{
	struct xh_data *xhd = xir_get_xh_data(xirr);
	if (!xhd) return NULL;

	return __xir_get_thread(xhd);
}

static inline int
xir_set_handler(xirr_t xirr, xirr_func_t h, void *data)
{
	struct xh_data *xhd = xirr_classes[xirr_class(xirr)].xc_data;

	return xirr_set_handler(xhd, xirr, h, data);
}

static inline uval
xir_set_flags(xirr_t xirr, uval f)
{
	struct xh_data *xhd = xirr_classes[xirr_class(xirr)].xc_data;

	return xirr_set_flags(xhd, xirr, f);
}

static inline uval
xir_get_flags(xirr_t xirr)
{
	struct xh_data *xhd = xirr_classes[xirr_class(xirr)].xc_data;

	return xirr_get_flags(xhd, xirr);
}


static inline struct cpu_thread *
xir_set_thread(xirr_t xirr, struct cpu_thread *thr)
{
	struct cpu_thread *t = NULL;
	struct xh_data *xhd = xir_get_xh_data(xirr);
	if (!xhd) return NULL;

	if (xhd->xh_flags & XIRR_DATA_THR) {
		t = xhd->xh_data;
	}
	xhd->xh_flags |= XIRR_DATA_THR;
	xhd->xh_data = thr;

	return t;
}

static inline void
xir_clear_handler(xirr_t xirr)
{
	struct xh_data *xhd = xirr_classes[xirr_class(xirr)].xc_data;

	xirr_clear_handler(xhd, xirr);
}

static inline int
xirq_empty(struct xir_queue *xq)
{
	return (xq->xq_head == xq->xq_tail) ? 1 : 0;
}

static inline xirr_t
xirq_peek(struct xir_queue *xq)
{
	if (xq->xq_head == xq->xq_tail) {
		return 0;
	}

	return xq->xq_list[xq->xq_head];
}

static inline xirr_t
xirq_pop(struct xir_queue *xq)
{
	int head = xq->xq_head;

	if (head == xq->xq_tail) {
		return 0;
	}
	if (++(xq->xq_head) >= XIRR_DEVID_SZ) {
		xq->xq_head = 0;
	}

	return xq->xq_list[head];
}

static inline int
xir_add(struct xir_queue *xq, xirr_t xirr)
{
	int tail = xq->xq_tail + 1;

	if (tail >= XIRR_DEVID_SZ) {
		tail = 0;
	}
	if (tail != xq->xq_head) {
		int ret = xq->xq_tail;

		xq->xq_list[ret] = xirr;
		xq->xq_tail = tail;
		return ret;
	}
	return -1;
}

extern void *xir_get_device(xirr_t xirr);
extern sval xir_default_config(xirr_t xirr, struct cpu_thread *dest,
			       void *device);

extern void xir_init_class(int class, xirr_func_t handler, void *arg);

extern int __xir_enqueue(xirr_t xirr, struct xh_data *xh,
			 struct cpu_thread *thread);

extern int xir_inject(uval class, uval dev_id,
		      uval payload, struct cpu_thread **receiver);

extern void xir_set_state(xirr_t xirr, struct xh_data* xd, uval state);

static inline int
xir_raise(xirr_t xirr, struct cpu_thread **receiver)
{
	return xir_inject(xirr_class(xirr), xirr_dev_id(xirr), 0, receiver);
}

extern void xir_eoi(struct cpu_thread *thread, xirr_t xirr);

/* Use xir_find if you want xir infrastructure to give you isrc
 * numbers, use xir_grab if you have your own system of managing
 * isrc's for a particular class */

/* Allocates and returns an isrc */
extern int xir_find(uval class);

/* Attempts to mark xirr as being used; returns 1 on success */
extern int xir_grab(xirr_t xirr);


extern int xirqs_empty(struct cpu_thread *thread);

#endif /* _XIR_H */
