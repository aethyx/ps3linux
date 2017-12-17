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
#include <hype.h>
#include <mmu.h>
#include <pmm.h>
#include <h_proto.h>

/*
 * TODO: implement these checks
 */

/* check if address is within the range for this partition */
#define CHECK_ADDRESS(os, addr)	1

/* check if address is mapped as cache inhibited */
#define CACHE_INHIBITED(os, addr)	1

/* check if address is mapped as cacheable */
#define CACHEABLE(os, addr)		1

#ifdef HAS_DABR
#define DABR_BT_MASK 0x4UL

uval
h_set_dabr(struct cpu_thread *thread, uval addr)
{
	uval tmp_dabr;

	tmp_dabr = thread->reg_dabr;
	if (tmp_dabr & DABR_BT_MASK) {
		/* DABR BT bit is set. DABR must be reserved by Hypervisor */
		return H_RESERVED_DABR;
	} else {
		/* DABR BT bit Breakpoint Translation) is not set in current DABR */
		/* Set new value and make sure BT is masked off */
		thread->reg_dabr = addr & ~DABR_BT_MASK;
	}
	/* Successful return */
	return H_Success;
}

#endif /* HAS_DABR */
