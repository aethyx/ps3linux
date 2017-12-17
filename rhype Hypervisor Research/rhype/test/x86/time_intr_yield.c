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
 * Test x86 interrupt semantics
 */

#include <test.h>
#include <lpar.h>
#include <hcall.h>
#include <mmu.h>

extern void timer_rupt_setup(void);

uval
test_os(uval argc __attribute__ ((unused)),
	uval argv[] __attribute__ ((unused)))
{
	hprintf("Setting clock interrupt vector ...\n"
		"You should see mbox ticks values every 10 seconds.\n"
	        "This test will run until manually halted.\n"
		"This test sits in a for loop spinning on hcall_yield\n");


	timer_rupt_setup();

	for (;;) {
		hcall_yield(NULL, H_SELF_LPID);
	}
	return 0;
}
