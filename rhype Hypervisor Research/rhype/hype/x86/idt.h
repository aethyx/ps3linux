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
#ifndef __HYPE_X86_IDT_H
#define __HYPE_X86_IDT_H

#include_next <idt.h>

#ifndef __ASSEMBLY__
#include <types.h>

struct idt_descriptor {
	uval32 offset;
	uval16 segment;
	uval16 flags;
};

struct cpu_thread;		/* forward declaration, avoids circularity */

static inline uval32
get_intr_handler_offset(struct idt_descriptor *descp)
{
	return descp->offset;
}

static inline uval16
get_intr_handler_segment(struct idt_descriptor *descp)
{
	return descp->segment;
}

static inline uval
get_idt_gate_type(struct idt_descriptor *descp)
{
	return descp->flags & GATE_MASK;
}

static inline uval
get_idt_gate_pl(struct idt_descriptor *descp)
{
	return ((descp->flags >> 13) & 0x3);
}

/* intr.c */
extern uval force_page_fault(struct cpu_thread *, uval, uval);
extern uval force_sync_exception(struct cpu_thread *, struct idt_descriptor *,
				 int);
extern void enable_intr(struct cpu_thread *);
extern void disable_intr(struct cpu_thread *);

/* idt.c */
extern void idt_init(struct cpu_thread *thread);

extern struct idt_descriptor *get_idt_descriptor(struct cpu_thread *thread,
						 uval vector);
extern uval pl_change_required(struct cpu_thread *thread,
			       struct idt_descriptor *descp);

/* pic.c */
extern void pic_init(void);
extern void pic_set_owner(struct cpu_thread *owner);
extern void pic_mask_and_ack(uval pic);
extern void pic_enable(uval irq);
extern void pic_disable(uval irq);
#endif /* __ASSEMBLY__ */

#endif /* __HYPE_X86_IDT_H */
