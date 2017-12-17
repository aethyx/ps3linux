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
#include <tce.h>
#include <lib.h>

sval
h_stuff_tce(struct cpu_thread *thread, uval32 liobn, uval ioba,
	    uval64 tce_dword, uval count)
{
	union tce tce;

	thread = thread;
	tce.tce_dword = tce_dword;
	hprintf("%s: liobn: 0x%x ioba: 0x%lx tce: 0x%llx(0x%llx) count: %lu\n",
		__func__, liobn, ioba, tce.tce_dword, tce.tce_bits.tce_rpn,
		count);

	return (H_Function);
}
