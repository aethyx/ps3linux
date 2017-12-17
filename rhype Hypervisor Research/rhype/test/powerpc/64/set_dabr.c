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

#ifdef HAS_DABR

	/* Allocate data buffer to point DABR at */
	unsigned watchbuf[1024];

	uval fault_detected = 0;
	uval dabr;
	uval retcode;
	uval ret[2];

	/* Test 1: Set DABR to 0 */
	dabr = 0;
	retcode = hcall_set_dabr(ret, dabr);
	if (retcode == H_RESERVED_DABR) {
		hputs("H_SET_DABR: Set to Zero FAILURE: H_RESERVED_DABR\n");
		fault_detected = 1;
	} else if (retcode != H_Success) {
		hputs("H_SET_DABR: Set to Zero FAILURE: Unexpected Return\n");
		fault_detected = 1;
	}

	/* Test 2: Set DABR to non-zero value  */
	dabr = (int64)&watchbuf;
	dabr = (dabr & (~0x7));
	retcode = hcall_set_dabr(ret, dabr);
	if (retcode == H_RESERVED_DABR) {
		hputs("H_SET_DABR: "
		      "Set to Non-Zero FAILURE: H_RESERVED_DABR\n");
		fault_detected = 1;
	} else if (retcode != H_Success) {
		hputs("H_SET_DABR: "
		      "Set to Non-Zero FAILURE: Unexpected Return\n");
		fault_detected = 1;
	}

	/* Test 3: Try to force BT bit on, then check by setting */
	/*         DABR back to 0                                */
	dabr = (int64)&watchbuf;
	dabr = ((dabr & (~0x3)) | (0x4));
	retcode = hcall_set_dabr(ret, dabr);
	if (retcode == H_RESERVED_DABR) {
		hputs("H_SET_DABR: "
		      "Force BT, part 1 FAILURE: H_RESERVED_DABR\n");
		fault_detected = 1;
	} else if (retcode != H_Success) {
		hputs("H_SET_DABR: "
		      "Force BT, part 1 FAILURE: Unexpected Return\n");
		fault_detected = 1;
	}

	/* Test 3, Part 2: If BT bit was set this attempt to set dabr */
	/*                 to Zero should fail.                       */
	dabr = 0;
	retcode = hcall_set_dabr(ret, dabr);
	if (retcode == H_RESERVED_DABR) {
		hputs("H_SET_DABR: "
		      "Force BT, part 2 FAILURE: H_RESERVED_DABR\n");
		fault_detected = 1;
	} else if (retcode != H_Success) {
		hputs("H_SET_DABR: "
		      "Force BT, part 2 FAILURE: Unexpected Return\n");
		fault_detected = 1;
	}

	if (fault_detected) {
		/* Fault detected, stop the simulator */
		return -1;
	}

#endif /* HAS_DABR */

	hputs("H_SET_DABR: SUCCESS\n");
	return 0;
}

