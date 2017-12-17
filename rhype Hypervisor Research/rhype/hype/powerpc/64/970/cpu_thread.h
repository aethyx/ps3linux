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

#ifndef _970_CPU_THREAD_H
#define _970_CPU_THREAD_H

#include <config.h>
#include <lpcr.h>
#include <partition.h>
#include <hid.h>
/* This is just a wrapper for including ../cpu_thread.h.
   This file defines cpu_imp_regs, a struct that contains all
   Book IV registers needed in the struct cpu_thread struct.
*/

struct cpu_imp_regs {
	union hid4 hid4;
};

#include_next <cpu_thread.h>

extern void imp_switch_to_thread(struct cpu_thread *thread);

static __inline__ uval
cpu_verify_rmo(uval rmo, uval rmo_size)
{
	if (rmo == 0) {
		return 0;
	}

	switch (rmo_size) {
	case 256ULL << 30:	/* 256 GB */
	case 16ULL << 30:	/* 16 GB */
	case 1ULL << 30:	/* 1 GB */
	case 64ULL << 20:	/* 64 MB */
	case 256ULL << 20:	/* 256 MB */
	case 128ULL << 20:	/* 256 MB */
		break;
	default:
		return 0;
	}

	if ((rmo / rmo_size) * rmo_size != rmo) {
		return 0;
	}

	return 1;
}

extern void imp_thread_init(struct cpu_thread *thr);

#endif /* _970_CPU_THREAD_H */

