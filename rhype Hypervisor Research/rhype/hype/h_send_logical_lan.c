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
#include <llan.h>

sval
h_send_logical_lan(struct cpu_thread *thread, uval uaddr,
		   uval64 bd1, uval64 bd2, uval64 bd3,
		   uval64 bd4, uval64 bd5, uval64 bd6, uval token)
{
	union tce_bdesc bd[] = {
		{.lbd_dword = bd1},
		{.lbd_dword = bd2},
		{.lbd_dword = bd3},
		{.lbd_dword = bd4},
		{.lbd_dword = bd5},
		{.lbd_dword = bd6}
	};

	return llan_send(thread, uaddr, bd, token);
}
