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
#ifndef __HYPE_X86_HV_REGS_H__
#define __HYPE_X86_HV_REGS_H__

#ifndef __ASSEMBLY__
#include <lib.h>
struct thread_control_area;

static inline struct thread_control_area *
get_tca(void)
{
	assert(0, "not implemented");
	return 0;
}

static inline uval32
get_tca_index(void)
{
	/* FIXME: Should assert, since get_tca does, but to get RCU
	 * to work, it's safe to return 0.
	 */
	return 0;
}

static inline uval32
get_proc_id()
{
	return 0;
}

static inline uval32
get_thread_id()
{
	return 0;
}

#endif /* __ASSEMBLY__ */

#endif /* __HYPE_X86_HV_REGS_H__ */
