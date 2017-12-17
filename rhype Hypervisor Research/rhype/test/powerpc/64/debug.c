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

#define TESTBUFSIZE 4096

struct partition_info pinfo[2] = {{
	.sfw_tlb = 1,
	.large_page_size1 = LARGE_PAGE_SIZE64K,
	.large_page_size2 = LARGE_PAGE_SIZE16M
},};

union test_bufs
{
	uval8	bufuval8[TESTBUFSIZE];
	uval16	bufuval16[TESTBUFSIZE/2];
	uval32	bufuval32[TESTBUFSIZE/4];
	uval64	bufuval64[TESTBUFSIZE/8];
};

union test_bufs	testbuf;

static uval
test_log_cache_store(void)
{
        int     index;
        uval   ret2[2];
        uval   retcode;
        uval   addr, value;

	/* 8 bit logical cache store and check test */
        /* Write pattern to testbuf using 8 bit writes */
        for (index = 0; index < TESTBUFSIZE; index++) {
                addr = V2P((uval)&testbuf.bufuval8[index]);
                value = index % 0xFFUL;
                retcode = hcall_logical_cache_store_64
                        (ret2, 1, addr, value);
                if (retcode) {
                        hputs("H_LOGICAL_CACHE_STORE: 8-bit store FAILURE\n");
			return retcode;
                }
        }

        /* Completed writing pattern using 8 bit writes */
        /* Check whether values in testbuf are correct */
        for (index = 0; index < TESTBUFSIZE; index++) {
                value = index % 0xFFUL;
                if (testbuf.bufuval8[index] != value) {
                        /* Mismatch of data value */
                        hputs("H_LOGICAL_CACHE_STORE: 8-bit compare FAILURE\n");
			return -1;
                }
        }

	/* 16 bit logical cache store and check test */
        /* Write pattern to testbuf using 16 bit writes */
        for (index = 0; index < TESTBUFSIZE/2; index++) {
                addr = V2P((uval)&testbuf.bufuval16[index]);
                value = index % 0xFFFFUL;
                retcode = hcall_logical_cache_store_64
                        (ret2, 2, addr, value);
                if (retcode) {
                        hputs("H_LOGICAL_CACHE_STORE: 16-bit store FAILURE\n");
			return retcode;
                }
        }

        /* Completed writing pattern using 16 bit writes */
        /* Check whether values in testbuf are correct */
        for (index = 0; index < TESTBUFSIZE/2; index++) {
                value = index % 0xFFFFUL;
                if (testbuf.bufuval16[index] != value) {
                        /* Mismatch of data value */
                        hputs("H_LOGICAL_CACHE_STORE 16-bit compare: "
			      "FAILURE\n");
                }
        }

	/* 32 bit logical cache store and check test */
        /* Write pattern to testbuf using 32 bit writes */
        for (index = 0; index < TESTBUFSIZE/4; index++) {
                addr = V2P((uval)&testbuf.bufuval32[index]);
                value = index % 0xFFFFUL;
                retcode = hcall_logical_cache_store_64
                        (ret2, 3, addr, value);
                if (retcode) {
                        hputs("H_LOGICAL_CACHE_STORE: 32-bit store FAILURE\n");
			return retcode;
                }
        }

        /* Completed writing pattern using 32 bit writes */
        /* Check whether values in testbuf are correct */
        for (index = 0; index < TESTBUFSIZE/4; index++) {
                value = index % 0xFFFFFFFFUL;
                if (testbuf.bufuval32[index] != value) {
                        /* Mismatch of data value */
                        hputs("H_LOGICAL_CACHE_STORE: "
			      "32-bit compare FAILURE\n");
                }
        }

#ifdef HAS_64BIT

	/* 64 bit logical cache store and check test */
        /* Write pattern to testbuf using 64 bit writes */
        for (index = 0; index < TESTBUFSIZE/8; index++) {
                addr = V2P((uval)&testbuf.bufuval64[index]);
                value = index % 0xFFFFFFFFFFFFFFFFUL;
                retcode = hcall_logical_cache_store_64
                        (ret2, 4, addr, value);
                if (retcode) {
                        hputs("H_LOGICAL_CACHE_STORE: 64-bit store FAILURE\n");
			return retcode;
                }
        }

        /* Completed writing pattern using 64 bit writes */
        /* Check whether values in testbuf are correct */
        for (index = 0; index < TESTBUFSIZE/8; index++) {
                value = index % 0xFFFFFFFFFFFFFFFFUL;
                if (testbuf.bufuval64[index] != value) {
                        /* Mismatch of data value */
                        hputs("H_LOGICAL_CACHE_STORE: 64 bit compare: FAILURE\n");
			return -1;
                }
        }

#endif /* HAS_64BIT */
	return 0;
} /* end test_log_cache_store */


uval
test_os(uval argc __attribute__ ((unused)),
	uval argv[] __attribute__ ((unused)))
{
	uval ret2[2];
	uval retcode;
	uval addrAndVal, addr, value;		
	uval i;

	addrAndVal = 0x300;
	addr = 0x1F8; /* This is iffy, but should work for now */
	value = (uval)0xA5C3F082B6D4E179;

	for (i = 1; i < 5; i++) {
	        retcode = hcall_logical_ci_load_64(ret2, i, addrAndVal);
		if (retcode) {
			hputs("H_LOGICAL_CI_LOAD: FAILURE\n");
			return retcode;
	        }
	}
	hputs("H_LOGICAL_CI_LOAD: SUCCESS\n");

        for (i = 1; i < 5; i++) {
                retcode = hcall_logical_ci_store_64
			(ret2, i, addr, value);
                if (retcode) {
                        hputs("H_LOGICAL_CI_STORE: FAILURE\n");
			return retcode;
                }
        }
	hputs("H_LOGICAL_CI_STORE: SUCCESS\n");

        for (i = 1; i < 5; i++) {
                retcode = hcall_logical_cache_load_64
			(ret2, i, addrAndVal);
                if (retcode) {
                        hputs("H_LOGICAL_CACHE_LOAD: FAILURE\n");
			return retcode;
                }
        }
	hputs("H_LOGICAL_CACHE_LOAD: SUCCESS\n");

	/* Stress test the hcall_logical_cache_store_64 function */
	retcode = test_log_cache_store();
	if (retcode) {
	    hputs("H_LOGICAL_CACHE_STORE: FAILURE\n");
	    return retcode;
	}
	hputs("H_LOGICAL_CACHE_STORE: SUCCESS\n");

        retcode = hcall_logical_icbi (ret2, addr);
	if (retcode) {
		hputs("H_LOGICAL_ICBI: FAILURE\n");
		return retcode;
	}
	hputs("H_LOGICAL_ICBI: SUCCESS\n");
 
	retcode = hcall_logical_dcbf (ret2, addr);
	if (retcode) {
		hputs("H_LOGICAL_DCBF: FAILURE\n");
		return retcode;
	}

	hputs("H_LOGICAL_DCBF: SUCCESS\n");
	return 0;
}

