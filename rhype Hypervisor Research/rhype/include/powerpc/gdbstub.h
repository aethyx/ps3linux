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

#ifndef _GDBSTUB_PPC_H
#define _GDBSTUB_PPC_H

/* This is the format cpu state is saved in by low-level exception handlers */
struct cpu_state {
	uval gpr[32];
	uval pc;
	union {
		uval msr;
		uval ps;
	};
	uval32 cr;
	uval32 xer;
	uval ctr;
	uval lr;
	uval dar;
	uval dsisr;

	uval hsrr0;
	uval hsrr1;
	uval hdec;
	uval curmsr;
};

#include_next <gdbstub.h>

#endif /* _GDBSTUB_PPC_H */
