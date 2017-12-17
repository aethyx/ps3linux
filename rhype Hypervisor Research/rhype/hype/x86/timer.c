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
 * 8254 programmable interrupt timer 
 *
 */

#include <io.h>
#include <lib.h>
#include <timer.h>
#include <idt.h>

#define C0_PORT		0x40
#define C1_PORT		0x41
#define C2_PORT		0x42
#define MCR_PORT	0x43

volatile uval64 ticks = 0;

void
timer_init(void)
{
	uval16 freq_divider = (TIMER_HZ + HZ / 2) / HZ;

	/* 0x34: counter 0, MSB after LSB, rate generator mode 2 */
	outb(MCR_PORT, 0x34);
	outb(C0_PORT, (uval8)(freq_divider & 0xff));
	outb(C0_PORT, (uval8)(freq_divider >> 8));
}

void
update_ticks(struct cpu_thread *thread)
{
	struct mailbox *mbox = thread->mbox;

	if (mbox != NULL) {
		if (mbox->ticks < ticks)
			mbox->irq_pending |= 1 << TIMER_IRQ;

		mbox->ticks = ticks;
	}
}
