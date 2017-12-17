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
#ifndef _POWERPC_64_HV_REGS_H
#define _POWERPC_64_HV_REGS_H

#include <powerpc/hv_regs.h>

#ifndef __ASSEMBLY__

#if THREADS_PER_CPU <= 1
static __inline__ void
powerpc64_thread_init(void)
{
	return;
}
#endif

#endif /* ! __ASSEMBLY__ */
#endif /* ! _POWERPC_64_HV_REGS_H */
