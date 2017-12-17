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
#include <mmu.h>
#include <sysipc.h>

/* we should parse the device tree instead */
uval log_htab_bytes = LOG_DEFAULT_HTAB_BYTES;

struct partition_info pinfo[2] __attribute__ ((weak)) = {{
	.large_page_size1 = LARGE_PAGE_SIZE16M,
	.large_page_size2 = LARGE_PAGE_SIZE1M
}, };
int do_map_myself __attribute__ ((weak)) = 1;

#ifdef DEBUG
static void
dump_pinfo(void)
{
	hprintf("Dump of pinfo @ %p (%p):\n"
		"  sysid            = 0x%x   0x%x\n"
		"  sfw_tlb          = 0x%x   0x%x\n",
		&pinfo[1], &pinfo[0],
		pinfo[1].lpid, pinfo[0].lpid,
		pinfo[1].sfw_tlb, pinfo[0].sfw_tlb);

	hprintf("  large_page_size1 = 0x%lx  0x%lx\n"
		"  large_page_size2 = 0x%lx  0x%lx\n",
		pinfo[1].large_page_size1, pinfo[0].large_page_size1,
		pinfo[1].large_page_size2, pinfo[0].large_page_size2);
};
#endif /* DEBUG */

uval decr_cnt;

extern char _end[];

uval
map_pages(uval raddr, uval eaddr, uval size)
{
	uval offset;
	uval l = 1;
	uval ls = SELECT_LG;
	uval lp = get_log_pgsize(&pinfo[1], l, ls);
	uval page_size;
	int slbe;

#if FORCE_4K_PAGES
	lp = 12;
	l = 0;
	lg = 0;
	slbe = slb_insert(eaddr, 0, 0);
#else
	slbe = slb_insert(eaddr, 1, SELECT_LG);
#endif

	page_size = 1U << lp;
	raddr = ALIGN_DOWN(raddr, page_size);
	eaddr = ALIGN_DOWN(eaddr, page_size);
#ifdef DEBUG
#define TRACE_PAGES 256
	hprintf("Each 'p' is %d pages 0x%lx in size:\n",
		TRACE_PAGES, page_size);
#endif /* DEBUG */

	/* TODO - KYLE only call pte_enter if we are SWTLB */
#define NOSWTLB

	for (offset = 0; offset < size; offset += page_size) {
#ifdef DEBUG
		if (((offset / page_size) % TRACE_PAGES) == 0 &&
		    offset > 0) {
			hputs("p");
		}
#endif

#ifdef SWTLB
		if (pinfo[1].sfw_tlb) {
#ifdef SWTLB_PRELOAD
			int ret;
			ret = tlb_enter(eaddr + offset,
					raddr + offset,
					PAGE_WIMG_M, l, ls);
			assert(ret >= 0,
			       "fail: tlb_enter() returned: %d\n", ptex);

#endif /* SWTLB_PRELOAD */
		} else
#endif /* SWTLB */
		{
			int ptex;
			uval ea = eaddr + offset;
			uval ra = raddr + offset;

			ptex = pte_find_entry(&pinfo[1], ea, l, ls);
			/* Check if it's in the first chunk, don't
			 * unmap ourselves, but allow for somebody to
			 * remap us by allowing the duplicate pte_enter. */
			if (ptex >= 0 && ea >= (1 << LOG_CHUNKSIZE)) {
				pte_remove(ptex);
			}
			ptex = pte_enter(&pinfo[1], ea, ra, PAGE_WIMG_M,
					 0,  /* not bolted */
					 l, ls);
			assert(ptex >= 0,
			       "fail: pte_enter() returned: %d\n", ptex);
#if 0
			if (ea > 0) {
				if (l) {
					tlbie_large(ea);
				} else {
					tlbie(ea);
				}
			}
#endif
		}
	}

#ifdef DEBUG
	if ((offset % (page_size * TRACE_PAGES)) ==
	    ((page_size * (TRACE_PAGES - 1)))) {
		hputs("p");
	}
#endif /* DEBUG */
	hputs("\n");
	return size;
}

static void
map_myself(uval of_start)
{
	uval raddr = 0;
	uval eaddr = (uval)&_text_start;
	uval size = pinfo[1].mem[0].size;
	(void)of_start;
#ifdef DEBUG
	dump_pinfo();
	hprintf("map_myself(): bytes: 0x%lx, raddr = 0x%lx, eaddr 0x%lx\n",
		size, raddr, eaddr);
	hprintf("slb_insert(0x%lx, %u)\n", eaddr, 0);
#endif /* DEBUG */

	map_pages(raddr, eaddr, size);
}



static void
os_wrapper(uval r3, uval r4, uval ofh, uval r6, uval old_msr)
{
	uval ret;
	uval argv[] = { r3, r4, ofh, r6 };
	hprintf("partition info is at 0x%lx\n", _partition_info);

	prom_init(ofh, old_msr);

	if (custom_init) {
		custom_init(r3, r4, ofh, r6);
	}

	ret = test_os(4, argv);

	hprintf("test_os returned 0x%lx\n", ret);
	for (;;) {
		hcall_send_async(NULL, CONTROLLER_LPID, IPC_SUICIDE, 0, 0, 0);
		hcall_yield(NULL, 0);
	}
}

void
assert_at_0(void)
{
	assert(0, "you branched to 0\n");
	for (;;);
}

#ifdef DEBUG
void
ex_stack_assert(void)
{
	assert(0, "Exception Stack corruption\n");
	for (;;);
}
#endif

extern void leap_to(uval r3, uval r4, uval r5, uval r6, uval old_msr,
		    void (*func)(uval, uval, uval, uval, uval),
		    uval new_msr);
void
crt_init(uval r3, uval r4, uval r5, uval r6)
{
    uval new_msr;
    uval msr = mfmsr();

    if (mfpir() == 0) {
	    /* clear bss, do this first for obvious reasons */
	    memset(__bss_start, 0, _end - __bss_start);
    }
    new_msr = MSR_ME | MSR_RI | MSR_EE;
#ifdef TARGET_LP64
    new_msr |= MSR_SF;
#ifdef HAS_MSR_ISF
    new_msr |= MSR_ISF;
#endif
#endif

    if (mfpir() == 0) {
	    hcall_cons_init(0); /* setup hprintf to use channel 0 */
    }

    if (do_map_myself) {
	    hputs("Going to map myself\n");
	    map_myself(r5);
	    new_msr |= MSR_IR | MSR_DR;
    }
    leap_to(r3, r4, r5, r6, msr, os_wrapper, new_msr);

    assert(0, "!!Returned from leap_to(test_os, 0x%lx)!!\n", new_msr);
}
