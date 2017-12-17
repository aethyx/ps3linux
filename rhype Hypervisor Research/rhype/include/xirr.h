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
#ifndef _XIRR_H
#define _XIRR_H

#include <types.h>

/* FIXME: This encoding is entirely made up, when we know that that
 * the ICC Pending register looks like we can adjust accordingly. */
#define XIRR_DEVID_SZ	0x100

typedef uval32 xirr_t;

static inline uval
__xirr_get_field(xirr_t x, uval width, uval shift)
{
	uval mask = ((1 << width) - 1);

	x >>= shift;
	x &= mask;
	return x;
}

static inline void
__xirr_set_field(xirr_t *x, uval val, uval width, uval shift)
{
	uval mask = ((1 << width) - 1);

	val &= mask;
	mask = ~(mask << shift);
	*x = *x & mask;
	val <<= shift;
	*x = *x | val;
}

#define XIRR_XISR	0
#define XIRR_DEV_ID	XIRR_XISR
#define XIRR_CLASS	1

#define __STR(a) a
#define APPEND(a,b) __STR(a)##b

#define XIRR_XISR_WIDTH		20
#define XIRR_XISR_LOG		0
#define XIRR_CLASS_WIDTH	4
#define XIRR_CLASS_LOG		20
#define XIRR_CPPR_WIDTH		8
#define XIRR_CPPR_LOG		24

#define GET_XISR(xirr)	__xirr_get_field((xirr), \
					 XIRR_XISR_WIDTH, XIRR_XISR_LOG)
#define SET_XISR(xirr, val)	__xirr_set_field(&(xirr), val, \
					 XIRR_XISR_WIDTH, XIRR_XISR_LOG)
#define GET_DEV_ID(xirr) GET_XISR(xirr)
#define SET_DEV_ID(xirr) SET_XISR(xirr)

#define GET_CLASS(xirr)	__xirr_get_field((xirr), \
					   XIRR_CLASS_WIDTH, XIRR_CLASS_LOG)
#define SET_CLASS(xirr, val)	__xirr_set_field(&(xirr), val, \
					   XIRR_CLASS_WIDTH, XIRR_CLASS_LOG)

#define GET_CPPR(xirr)	__xirr_get_field((xirr), \
					 XIRR_CPPR_WIDTH, XIRR_CPPR_LOG)
#define SET_CPPR(xirr, val)	__xirr_set_field(&(xirr), val, \
					 XIRR_CPPR_WIDTH, XIRR_CPPR_LOG)

static inline xirr_t
xirr_encode(uval dev_id, uval class)
{
	xirr_t x = 0;

	SET_XISR(x, dev_id);
	SET_CLASS(x, class);
	return x;
}

static inline uval8
xirr_cppr(xirr_t xirr)
{
	return GET_CPPR(xirr);
}

static inline uval
xirr_dev_id(xirr_t xirr)
{
	return GET_XISR(xirr);
}

static inline uval8
xirr_class(xirr_t xirr)
{
	return GET_CLASS(xirr);
}

static inline xirr_t
xirr_xisr(xirr_t xirr)
{
	return GET_XISR(xirr);
}

struct cpu_thread;
struct xh_data;

typedef int (xirr_func_t) (xirr_t xirr, struct xh_data *xh, uval payload,
			   struct cpu_thread **receiver);

#define XIRR_INVALID	0xffffffff
#define XIRR_DISABLED		0x1
#define XIRR_PENDING		0x2
#define XIRR_OUTSTANDING	0x4
#define XIRR_DATA_THR		0x8	/* Data is thread identifier */
#define XIRR_INUSE	       0x10
#define __XIRR_LOCK_BIT	 0x80000000

struct xh_data {
	void *xh_data; /* if XIRR_DATA_THR, then this is a thread pointer */
	xirr_func_t *xh_handler;
	void *xh_device; /* device-specific data (e.g. pointer to llan device*/
	uval32 xh_flags;
};

extern struct xh_data *xh_data;

static inline struct xh_data *
xirr_ptr(struct xh_data *xhd, xirr_t xirr)
{
	return &xhd[xirr_dev_id(xirr)];
}

extern int xirr_set_handler(struct xh_data *xhd, xirr_t xirr,
			    xirr_func_t * h, void *d);
extern void *xirr_get_data(struct xh_data *xhd, xirr_t xirr);
extern void *xirr_set_data(struct xh_data *xhd, xirr_t xirr, void *data);
extern uval xirr_set_flags(struct xh_data *xhd, xirr_t xirr, uval f);
extern uval32 xirr_get_flags(struct xh_data *xhd, xirr_t xirr);
extern void xirr_clear_handler(struct xh_data *xhd, xirr_t xirr);
extern int xirr_handle(struct xh_data *xhd, xirr_t xirr, uval payload,
		       struct cpu_thread **receiver);

extern void __xirr_lock(struct xh_data *xhd);
extern void __xirr_unlock(struct xh_data *xhd);
extern void xirr_lock(struct xh_data *xhd, xirr_t xirr);
extern void xirr_unlock(struct xh_data *xhd, xirr_t xirr);

#endif /* _XIRR_H */
