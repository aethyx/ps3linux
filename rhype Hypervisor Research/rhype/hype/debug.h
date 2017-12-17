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
#ifndef __HYPE_DEBUG_H__
#define __HYPE_DEBUG_H__

#include_next <debug.h>

#ifdef DEBUG
#undef DEBUG_MT
#define DEBUG_MT(level, fmt, args...)					\
	if (DEBUG_ACTIVE(level)) {					\
		(*debugs[level].output_fn)("%x-%x %s: " fmt,		\
					   get_proc_id(), get_thread_id(),\
					   debugs[level].prefix, ## args); \
	}

#undef DEBUG_OVERRIDE
#define DEBUG_OVERRIDE(level, func, fmt, args...)			\
	if (DEBUG_ACTIVE(level)) {					\
		func("%x-%x %s: " fmt,					\
		     get_proc_id(), get_thread_id(),			\
		     debugs[level].prefix, ## args);			\
	}
#endif

#include <cpu_thread.h>
extern uval probe_reg(struct cpu_thread *thread, uval idx);

#endif /* __HYPE_DEBUG_H__ */
