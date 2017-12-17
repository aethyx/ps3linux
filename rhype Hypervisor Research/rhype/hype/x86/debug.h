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
/*
 * Useful debug routines.
 *
 */
#ifndef __HYPE_X86_DEBUG_H__
#define __HYPE_X86_DEBUG_H__

#include_next <debug.h>

#ifdef DEBUG
extern void dump_stack(struct cpu_thread *thread, int low, int high);
extern void dump_cpu_state(struct cpu_thread *);
extern void hexdump(unsigned char *data, int sz);
#endif /* DEBUG */

#endif /* __HYPE_X86_DEBUG_H__ */
