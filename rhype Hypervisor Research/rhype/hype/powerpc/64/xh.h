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

#ifndef _POWERPC_64_XH_H
#define _POWERPC_64_XH_H
#include_next <xh.h>

#define	XH_SYSRESET	0x0000
#define	XH_MACHCHECK	0x0001
#define	XH_DSI		0x0002
#define	XH_DATA_SLB	0x0003
#define	XH_ISI		0x0004
#define	XH_INST_SLB	0x0005
#define	XH_INTERRUPT	0x0006
#define	XH_ALIGNMENT	0x0007
#define	XH_PROGRAM	0x0008
#define	XH_FLOAT	0x0009
#define	XH_DEC		0x000a
#define	XH_HDEC		0x000b
#define	XH_SYSCALL	0x000c
#define	XH_TRACE	0x000d
#define	XH_FP		0x000e

#endif /* _POWERPC_64_XH_H */
