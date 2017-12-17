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

#ifndef _FLOAT_H
#define _FLOAT_H

#if defined(HAS_FP) || defined(HAS_VMX)

extern void save_float(struct cpu_thread *);
extern void restore_float(struct cpu_thread *);

extern void switch_float(struct cpu_thread *cur_thread,
			 struct cpu_thread *next_thread);

#else  /* defined(HAS_FLOAT) || defined(HAS_VMX) */

static inline void
switch_float(struct cpu_thread *cur_thread, struct cpu_thread *next_thread)
{
	return;
}

#endif /* defined(HAS_FLOAT) || defined(HAS_VMX) */
#endif /* _FLOAT_H */
