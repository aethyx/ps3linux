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
#include <h_proto.h>
#include <os.h>
#include <crq.h>
#include <debug.h>

sval
h_send_crq(struct cpu_thread *thread, uval uaddr, uval64 msg_hi, uval64 msg_lo)
{
	unsigned char msg[16];
	struct os *os = thread->cpu->os;

	DEBUG_OUT(DBG_CRQ, "%s: uaddr: 0x%lx msg_hi: 0x%llx msg_lo: 0x%llx\n",
		__func__, uaddr, msg_hi, msg_lo);

	memcpy(&msg[0], &msg_hi, sizeof (msg_hi));
	memcpy(&msg[8], &msg_lo, sizeof (msg_lo));

	return crq_send(os, uaddr, msg);
}
