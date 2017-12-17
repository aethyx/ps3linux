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

#include <config.h>
#include <lib.h>
#include <boot.h>
#include <asm.h>
#include <hv_regs.h>
#include <hid.h>
#include <io.h>
void cpu_boot_init(uval r3, uval r4, uval r5, uval r6, uval r7, uval boot_msr);

void
cpu_boot_init(uval r3, uval r4, uval r5, uval r6, uval r7, uval boot_msr)
{

	(void)r3;
	(void)r4;
	(void)r5;
	(void)r6;
	(void)r7;
	(void)boot_msr;

	/* Attempt to turn on external time-base */
	io_out8((volatile uval8 *)0x80000073, 5);

	/* This uses external tb */
	/* set_hid0(0x0011118100000000); */

	/* Internal tb */
	set_hid0(0x0011018100000000);

	set_hid1(0xfd3c200000000000);
	set_hid4(0x0000001000000004);
	set_hid5(0x0000000000000080);
}
