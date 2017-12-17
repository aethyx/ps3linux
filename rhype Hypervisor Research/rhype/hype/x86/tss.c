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
#include <lib.h>
#include <pmm.h>
#include <hcall.h>
#include <h_proto.h>

sval
h_sys_stack(struct cpu_thread *thread, uval ss, uval esp)
{
	uval ss_idx;

#ifdef SYS_STACK_DEBUG
	hprintf("h_sys_stack: lpid 0x%x: ss 0x%lx, esp 0x%lx\n",
		thread->cpu->os->po_lpid, ss, esp);
#endif
	/*
	 * The segment must be in the client-controllable range in the gdt.
	 * Entries in that range are enforced to be in ring 2 or 3.
	 */
	ss_idx = (ss >> 3);
	if ((ss_idx < GDT_ENTRY_MIN) || (ss_idx >= GDT_ENTRY_COUNT)) {
		return H_Parameter;
	}

	thread->tss.ss2 = ss;
	thread->tss.esp2 = esp;

	return H_Success;
}
