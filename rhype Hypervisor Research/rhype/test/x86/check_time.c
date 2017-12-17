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

#include <test.h>

uval
test_os(uval argc __attribute__ ((unused)),
	uval argv[] __attribute__ ((unused)))
{
	uval old;
	uval printed;

	hputs("You should see mbox ticks values every 10 seconds.\n"
	      "This test will run until manually halted.\n"
	      "This test uses a while loop to spin on hcall_yield().\n"
	      "no interrupts enabled\n");

	printed = mbox.ticks / 1000;

	while (1) {
		do {
			old = mbox.ticks;
		} while (!cas_uval((volatile uval*)&(mbox.ticks), old, 0));

		if (printed < (old / 1000)) {
			hprintf("%lu  ", old);
			printed = old / 1000;
		}

		hcall_yield(NULL, H_SELF_LPID);
	}
	return 0;
}
