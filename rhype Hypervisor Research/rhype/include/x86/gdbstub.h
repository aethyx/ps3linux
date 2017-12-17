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

#ifndef _GDBSTUB_x86_H
#define _GDBSTUB_x86_H

#include <types.h>

/* regs in the format gdb expects them
 *
 * note: gdb_read_reg_array() and gdb_write_to_packet_reg_array()
 *       assume that all struct members are sizeof uval
 */
struct cpu_state {
	uval eax;
	uval ecx;
	uval edx;
	uval ebx;
	uval esp;
	uval ebp;
	uval esi;
	uval edi;
	uval eip;
	uval eflags;
	uval cs;
	uval ss;
	uval ds;
	uval es;
	uval fs;
	uval gs;
};

#define NR_CPU_STATE_REGS (sizeof(struct cpu_state) / sizeof(uval32))

#include_next <gdbstub.h>

#endif
