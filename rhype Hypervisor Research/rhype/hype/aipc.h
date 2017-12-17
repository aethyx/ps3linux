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
#ifndef __HYPE_AIPC_H
#define __HYPE_AIPC_H

#include <ipc.h>
#include <xirr.h>
struct aipc {
	struct msg_queue *buf;	/* async msg buffer */
	uval buf_size;		/* buffer size      */
	xirr_t xirr;
	lock_t lock;		/* lock */
	struct cpu_thread *target;
};

/* Relies on per-os lock */
struct aipc_services {
	struct aipc *aipc[MAX_NUM_AIPC];
	struct lpar_event *cleanup;
};

struct os;
extern sval ipc_send_async(struct cpu_thread *dest, uval sender,
			   uval arg1, uval arg2, uval arg3, uval arg4);

extern sval ipc_directed_aipc(struct os *dest, uval aipc_id,
			      uval src, uval arg1, uval arg2,
			      uval arg3, uval arg4);

extern sval aipc_create(struct cpu_thread *thread,
			uval lbase, uval size, uval flags);

extern uval aipc_sys_init(void);
#endif /* __HYPE_AIPC_H */
