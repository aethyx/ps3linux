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
#ifndef _IPC_H
#define _IPC_H

#include <types.h>

/* Define a set of messages for TestOS's to use */
enum {
	IPC_REGISTER_SELF = 0x0,	/* tell controller we'll accept msgs */
	IPC_LPAR_RUNNING = 0x1,	/* tell OS about existence of another */
	IPC_LPAR_DEAD = 0x2,	/* notification of LPAR death */
	IPC_SUICIDE = 0x3,	/* ask controller to kill me */
	IPC_RESOURCES = 0x4,	/* resources have been given */
};

#define MAX_NUM_AIPC	16 /* Number of aipc bufs per partition */

#define AIPC_LOCAL	1

struct async_msg_s {
	uval am_source;
	union {
		struct {
			uval am_mtype;
			union {
				struct {
					uval amr_lpid;
				} amt_running;
				struct {
					uval amd_lpid;
				} amt_dead;
				struct {
					uval ams_type;
					uval ams_laddr;
					uval ams_size;
				} amt_resources;
			} amt_data;
		};
		uval amu_data[4];
	} am_data;
};

struct msg_queue {
	uval bufSize;
	uval head;
	uval tail;
	struct async_msg_s buffer[0];
};

#endif /* _IPC_H */
