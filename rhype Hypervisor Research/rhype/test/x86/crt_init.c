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
#include <partition.h>
#include <sysipc.h>	/* hcall_send_async stuff */
#include <mailbox.h>
#include <mmu.h>
#include <util.h>
#include <pgalloc.h>


enum {
	CHUNK_SIZE	= 1UL << LOG_CHUNKSIZE,
	NUM_TBL_PTE	= PGSIZE/sizeof(uval) /* how many pte's per table */
};

uval pgdir[NUM_TBL_PTE] __attribute__((aligned(PGSIZE)));
uval pgtbl[NUM_TBL_PTE][NUM_TBL_PTE] __attribute__((aligned(PGSIZE)));

static uval
__map_pages(uval raddr, uval eaddr, uval size)
{
	uval end = eaddr + size;
	raddr = ALIGN_DOWN(raddr, PGSIZE);
	eaddr = ALIGN_DOWN(eaddr, PGSIZE);
	size = ALIGN_UP(end - eaddr, PGSIZE);
	while (size) {
		size -= PGSIZE;

		uval ptex = ((eaddr + size) & PTE_MASK) >> LOG_PGSIZE;
		uval pdx  = ((eaddr + size) & PDE_MASK) >> LOG_PDSIZE;
		uval *ptbl;

		if (pdx >= NUM_TBL_PTE-1) {
			return (uval)H_Parameter;
		}

		ptbl = (uval*) pte_pageaddr(pgdir[pdx]);
		pgdir[pdx] |= PTE_P;
		ptbl[ptex] = (raddr + size) | PTE_P | PTE_RW | PTE_US;
	}
	return 0;
}

uval
map_pages(uval raddr, uval eaddr, uval size)
{
	uval ret = __map_pages(raddr, eaddr, size);
	if (ret != 0) {
		return ret;
	}
	hcall_page_dir(NULL, H_PAGE_DIR_PREFETCH, (uval)&pgdir[0]);

	/* Is TLB flush necessary? */
	uval i = 0;
	while (i < size) {
		hcall_flush_tlb(NULL,
				H_FLUSH_TLB_PREFETCH | H_FLUSH_TLB_SINGLE,
				eaddr + i);
		i+= PGSIZE;
	}
	return 0;
}

static void
init_pgtbl()
{
	sval i = 0;

	for (i = 0; i < NUM_TBL_PTE; ++i) {
		pgdir[i] = ((uval)&pgtbl[i][0]) | PTE_RW | PTE_US;
	}

	__map_pages(0, 0, CHUNK_SIZE);

	i = hcall_page_dir(NULL, H_PAGE_DIR_ACTIVATE, (uval)&pgdir[0]);
	assert(i == H_Success, "h_page_dir failed: %lx\n", i);

}


uval decr_cnt; /* XXX keep me? see test/sched.c */

struct partition_info pinfo[2] __attribute__ ((weak)) = {{
	.large_page_size1 = LARGE_PAGE_SIZE16M,
	.large_page_size2 = LARGE_PAGE_SIZE1M,
},};

void
halt(void)
{
	hcall_send_async(NULL, CONTROLLER_LPID, IPC_SUICIDE, 0, 0, 0);
	for (;;)
		hcall_yield(NULL, 0);
}

static void
os_wrapper(void)
{
	uval retcode, argv[1];

	argv[0] = 0;
	retcode = test_os(1, argv);

	halt();
}

extern char _end[];

void
crt_init(void)
{
	/* clear bss, do this first for obvious reasons */
	memset(__bss_start, 0, _end - __bss_start);

	hcall_cons_init(0);

#ifndef RELOADER
	setup_idt();
#endif
	init_pgtbl();
	rrupts_on();

	os_wrapper();
	/* not reached */
}

