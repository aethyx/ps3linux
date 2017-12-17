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
#ifndef __HYPE_VDEC_H
#define __HYPE_VDEC_H

#include <config.h>
#include <types.h>
#ifndef __ASSEMBLY__

extern void partition_set_dec(struct cpu_thread *thr, uval32 val);
extern sval xh_dec(struct cpu_thread *thread);
extern void sync_from_dec(void);
extern void sync_to_dec(struct cpu_thread* thread);


#endif

#endif /* __HYPE_VREGS_H */
