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
 * "Smoke" test of H_SET_ASR hcalls.
 */

#include <test.h>

struct partition_info pinfo[2] = {{
	.large_page_size1 = LARGE_PAGE_SIZE64K,
	.large_page_size2 = LARGE_PAGE_SIZE16M
},};


uval
test_os(uval argc __attribute__ ((unused)),
	uval argv[] __attribute__ ((unused)))
{

        uval asr_val, *logical_asr_val=0, orig_asr_val;
        uval retcode, *ret = 0;
	
	/* store original ASR contents */

	orig_asr_val = mfasr();

        /* set ASR to 0x50000 and check against the value */

        asr_val = 0x50000UL;
        retcode = hcall_set_asr(ret,asr_val);

        if (retcode != H_Success) {
                hputs("H_SET_ASR: Set to Zero FAILURE: Unexpected Return\n");
		return retcode;
        }
	
	hcall_real_to_logical(logical_asr_val, asr_val); 
	
	if ((uval)(*logical_asr_val) != mfasr()) {
		hputs("H_SET_ASR: FAILURE");
		return -1;
	}

	/* set the asr back to what it was */
	mtasr(orig_asr_val);

	hputs("H_SET_ASR: SUCCESS");
	return 0;
}

