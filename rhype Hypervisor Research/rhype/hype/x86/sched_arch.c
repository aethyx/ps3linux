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
 *
 */

#include <hype.h>
#include <os.h>
#include <timer.h>

/*
 * The decrementer ticks away at the configured clock rate. On each
 * clock tick the decrementer is decreased and when it hits zero
 * zero it will trigger a reschedule. 
 */
sval32 decrementer = DECREMENTER;

sval32
read_hype_timer(void)
{
	return 0;
}

int
get_rtos_dec(struct cpu_thread *rtos_thread __attribute__ ((unused)))
{
	return 0;
}
