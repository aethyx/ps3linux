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

#ifndef _VIO_H
#define _VIO_H

#include <config.h>
#include <types.h>
#include <cpu.h>
#include <tce.h>
#include <xirr.h>
#include <lpar.h>
#include <resource.h>

#define HVIO_CLASS_LOG   XIRR_CLASS_LOG
#define HVIO_CLASS_MASK	(((1U << XIRR_CLASS_WIDTH) - 1U) << XIRR_CLASS_LOG)
#define HVIO_DATA_MASK	((1U << XIRR_CLASS_WIDTH) - 1U)

static inline uval
vio_type(uval val)
{
	return (((uval32)val) & HVIO_CLASS_MASK) >> HVIO_CLASS_LOG;
}

static inline uval
vio_type_set(uval val, uval type)
{
	val &= HVIO_DATA_MASK;
	val |= ((type << HVIO_CLASS_LOG) & HVIO_CLASS_MASK);
	return val;
}


extern sval vio_ctl(struct cpu_thread *thread, uval cmd,
		    uval type, uval dma_range);
extern sval vio_signal(struct os *os, uval uaddr, uval mode);
extern sval vio_tce_put(struct os *os, uval32 liobn, uval ioba, union tce tce);

struct vios_resource {
	uval vr_liobn;
	struct sys_resource vr_res;
};

struct vios {
	char vs_name[16];
	sval (*vs_tce_put)(struct os *os, uval32 liobn,
			   uval ioba, union tce tce);
	sval (*vs_signal)(struct os *os, uval uaddr, uval mode);
	sval (*vs_acquire)(struct cpu_thread *thread, uval dma_range);
	sval (*vs_release)(struct os *os, uval32 liobn);
	sval (*vs_grant)(struct sys_resource **res,
			 struct os *os, struct os *dst, uval liobn);
	sval (*vs_accept)(struct os *os, uval liobn, uval *retval);
	sval (*vs_invalidate)(struct os *os, uval liobn);
	sval (*vs_return)(struct os *os, uval liobn);
	sval (*vs_rescind)(struct os *os, uval liobn);
	sval (*vs_conflict)(struct sys_resource *res1,
			    struct sys_resource *res2,
			    uval liobn);
};

extern sval vio_register(struct vios *vios, uval type);
extern void vio_init(void);

#endif /* ! _VIO_H */
