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

#ifndef _CPU_THREAD_INLINES_H
#define _CPU_THREAD_INLINES_H

#include <cpu_thread.h>

#if !defined(HAS_MEDIATED_EE) && !defined(FORCE_APPLE_MODE)
/* Mediated interrupts not implemented */
static inline uval
thread_set_MER(struct cpu_thread *thr, uval val)
{
	(void)thr;
	(void)val;
	return 0;
}
#endif

#ifdef FORCE_APPLE_MODE
static inline uval
thread_set_MER(struct cpu_thread *thr, uval val)
{
	if (val) {
		thr->vstate.thread_mode |= VSTATE_PENDING_EXT;
	} else {
		thr->vstate.thread_mode &= ~VSTATE_PENDING_EXT;
	}
	get_tca()->vstate = thr->vstate.thread_mode;
	return 0;
}
#endif /* FORCE_APPLE_MODE */
#endif /* _CPU_THREAD_INLINES_H */
