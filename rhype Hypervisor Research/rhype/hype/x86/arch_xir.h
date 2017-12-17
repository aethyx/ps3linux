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
 * Definitions for arch-dependent xir hooks.
 *
 */

#ifndef _X86_ARCH_XIR_H
#define _X86_ARCH_XIR_H

#include <idt.h>
#include <cpu_thread.h>

static inline void
arch_xir_raise(struct cpu_thread *thread)
{
	thread->mbox->irq_pending |= (1 << XIR_IRQ);
}

static inline void
arch_xir_eoi(struct cpu_thread *thread)
{
	thread->mbox->irq_masked &= ~(1 << XIR_IRQ);
}

#endif /* _X86_ARCH_XIR_H */
