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
 *
 * Timer specific definitions
 *
 */
#ifndef __HYPE_X86_TIMER_H__
#define __HYPE_X86_TIMER_H__

#include_next <timer.h>

/* timer interrupts to be generated per sec */
#define HZ		100

/* timer ticks per decrementer (reschedule) interrupt */
#define	DECREMENTER	10

/* freq. of the clock to the timer chip */
#define TIMER_HZ	1193182

extern volatile uval64 ticks;
extern sval32 decrementer;

extern void timer_init(void);

#endif /* __HYPE_X86_TIMER_H__ */
