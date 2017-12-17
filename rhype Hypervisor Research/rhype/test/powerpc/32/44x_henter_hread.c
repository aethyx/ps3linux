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
#include <lpar.h>
#include <types.h>
#include <sim.h>
#include <lib.h>
#include <hcall.h>

static void
tw_puts(char *str)
{
	hcall_put_term_buffer(NULL, 0, strlen(str), V2P(str));
}

static void
tw_puthex(uval val)
{
	unsigned char buf[9];
	int i;

	for (i = 7;  i >= 0;  i--) {
		buf[i] = "0123456789ABCDEF"[val & 0x0F];
		val >>= 4;
	}
	buf[8] = '\0';
	tw_puts(buf);
}

void
test_os(void)
{
	uval tlbw0, tlbw1, tlbw2;
	uval flags = 0;
	uval index = 5;
	uval ret2[3];
	uval retcode;

	tlbw0 = 0x00000801;
	tlbw1 = 0x33330002;
	tlbw2 = 0x00040005;

	retcode = hcall_enter(ret2, flags, index, tlbw0, tlbw1, tlbw2);
	if (retcode) {
		tw_puts("H_ENTER: FAILURE\n");
		tw_puts("H_ENTER returned 0x"); tw_puthex(retcode); tw_puts("\n");
		while(1);
	}
	index = ret2[0];

	retcode = hcall_read(ret2, flags, index);
	if (retcode) {
		tw_puts("H_READ: FAILURE\n");
		tw_puts("H_READ returned 0x"); tw_puthex(retcode); tw_puts("\n");
		while(1);
	}

	/* Test that registers entered into page table match what was read out */
	if (tlbw0 != ret2[0] || tlbw1 != ret2[1] || tlbw2 != ret2[2]) {
		tw_puts("H_ENTER/H_READ: FAILURE\n");
		tw_puts("expected 0x"); tw_puthex(tlbw0); tw_puts("\n");
		tw_puts("         0x"); tw_puthex(tlbw1); tw_puts("\n");
		tw_puts("         0x"); tw_puthex(tlbw2); tw_puts("\n");
		tw_puts("read     0x"); tw_puthex(ret2[0]); tw_puts("\n");
		tw_puts("         0x"); tw_puthex(ret2[1]); tw_puts("\n");
		tw_puts("         0x"); tw_puthex(ret2[2]); tw_puts("\n");
	}
	else {	
		tw_puts("H_ENTER/H_READ: SUCCESS\n");
	}
	while(1);
}

