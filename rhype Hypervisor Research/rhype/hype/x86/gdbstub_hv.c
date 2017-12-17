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

#include <pmm.h>
#include <mmu.h>
#include <cpu.h>
#include <vm.h>
#include <hype.h>
#include <gdbstub.h>

static uval __gdbstub
_hype_gdb_xlate_ea(uval eaddr)
{
	uval phys;

	phys = physical(eaddr);
	if (phys == INVALID_VIRTUAL_ADDRESS && get_cur_tss() != NULL) {
		phys = v2p((struct cpu_thread *)get_thread_addr(), eaddr);
	}
	return phys;
}

extern uval __gdbstub gdb_xlate_ea(uval eaddr)
	__attribute__ ((alias("_hype_gdb_xlate_ea")));
