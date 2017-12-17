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


#ifndef DEBUG_H
#define DEBUG_H

#include <types.h>

typedef sval (*debug_out_fn)(const char *fmt, ...);

extern uval debug_level;

struct debug_info {
	const char *prefix;
	debug_out_fn output_fn;
};

enum {
 	DBG_INTRPT  = 0,  /* external interrupt handling */
 	DBG_MEMMGR  = 1,  /* memory manager (LA manangement) */
 	DBG_LPAR    = 2,  /* partition management */
 	DBG_TCA     = 3,  /* thread control area */
 	DBG_INIT    = 4,  /* initialization stuff */
 	DBG_VM_MAP  = 5,  /* h_vm_map */
 	DBG_EE      = 6,
 	DBG_LLAN    = 7,
 	DBG_VTERM   = 8,
 	DBG_CRQ     = 9,
};

extern struct debug_info debugs[];


#ifdef DEBUG
#define DEBUG_ACTIVE(level) (debug_level & (1ULL<<level))
#define DEBUG_OUT(level, fmt, args...) do {				\
	if (DEBUG_ACTIVE(level)) {					\
		(*debugs[level].output_fn)("%s: " fmt,			\
					   debugs[level].prefix, args); \
	}								\
} while (0)

#define DEBUG_MT(level, fmt, args...)  DEBUG_OUT(level, fmt, args)

/* In case you want to provide a specific function to deal with this */
#define DEBUG_OVERRIDE(level, func, fmt, args...) do {			\
	if (DEBUG_ACTIVE(level)) {					\
		func("%s: " fmt,					\
		     debugs[level].prefix, ## args);			\
	}								\
} while (0)

#else
#define DEBUG_ACTIVE(level) (0)
#define DEBUG_OUT(level, fmt, args...)
#define DEBUG_MT(level, fmt, args...)
#define DEBUG_OVERRIDE(level, func, fmt, args...)
#endif

#endif /* !HYPE_DEBUG_H */
