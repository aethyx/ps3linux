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
#include <h_proto.h>
sval
h_set_asr(struct cpu_thread *thread __attribute__ ((unused)),
	  uval value __attribute__ ((unused)))
{
#if HAS_ASR
	uval L_addr;

	/* FIXME: is 0 right? */
	L_addr = logical_to_physical_address(thread->cpu->os, value, 0);

	if (L_addr == INVALID_PHYSICAL_ADDRESS) {
		return (H_Parameter);
	}

	thread->reg_asr = L_addr;

	mtasr(thread->reg_asr);
	return (H_Success);
#else
	return (H_Function);
#endif
}
