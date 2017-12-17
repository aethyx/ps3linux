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

#include <asm.h>
#include <types.h>
#include <hv_regs.h>
#include <hype.h>

sval32
read_hype_timer(void)
{
	return mfhdec();
}

sval32
get_rtos_dec(struct cpu_thread *rtos_thread)
{
	sval32 tb_delta;
	sval32 dec;

	tb_delta = mftb() - rtos_thread->reg_tb;
	dec = rtos_thread->reg_dec - tb_delta;

	return dec;
}
