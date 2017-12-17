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
 * Test copy on write, and the setting of the dirty bit.
 * (quick and dirty, sorry)
 */
#include <test.h>
#include <lpar.h>
#include <hcall.h>
#include <mmu.h>
#include <idt.h>

extern uval pgdir[];
extern void cow_handler(void);
extern void do_cow(uval ax, uval cx, uval dx, uval bx,
		   uval ebp, uval esi, uval edi,
		   uval ret, uval err, uval eip, uval cs, uval eflags);

void
do_cow(uval ax, uval cx, uval dx, uval bx,
       uval ebp, uval esi, uval edi,
       uval retpc, uval err, uval eip, uval cs, uval eflags)
{
	uval i;
	uval cr2;
	uval ret[1];
	uval *pgtbl;

	/* these are currently unused.. but could be */
	ax = ax;
	cx = cx;
	dx = dx;
	bx = bx;
	ebp = ebp;
	esi = esi;
	edi = edi;
	err = err;
	eip = eip;
	cs = cs;
	eflags = eflags;
	retpc = retpc;

	hprintf("(CoW pagefault) ");
	hcall_get_pfault_addr(&cr2);
	pgtbl = (uval *)pte_pageaddr(pgdir[cr2 >> LOG_PDSIZE]);
	pgtbl[(cr2 >> LOG_PGSIZE) & 0x3FF] |= 2;	/* write */
	i = hcall_flush_tlb(ret, H_FLUSH_TLB_SINGLE, cr2);
	assert(i == H_Success, "hcall_flush_tlb failed\n");
}

uval
test_os(uval argc __attribute__ ((unused)),
	uval argv[] __attribute__ ((unused)))
{
	uval new_vma;
	uval old_vma;
	uval lma;
	uval *pgtbl;
	uval *ptr1;
	uval *ptr2;
	uval i;
	uval j;
	uval ret[1];

	/*
	 * We're running on a page table that maps CHUNKSIZE bytes
	 * starting at 0, with vma == lma.  See crt_init.c.
	 *
	 * We remap the last 4M read-only at vma CHUNKSIZE.
	 *
	 * Do an hcall_page_dir to reinstall cr3.
	 *
	 * Then write the values through vma CHUNKSIZE and read them through
	 * vma (CHUNKSIZE - 4M).  The values should be consistent and the
	 * dirty bits should be set in the new mappings.
	 */

	new_vma = (1ul << LOG_CHUNKSIZE);
	old_vma = new_vma - 0x400000;
	lma = old_vma;

	pgdir[new_vma >> LOG_PDSIZE] |= PTE_P;
	pgtbl = (uval *)pte_pageaddr(pgdir[new_vma >> LOG_PDSIZE]);

	hprintf("pgdir: %p, pgtbl: %p\n", pgdir, pgtbl);

	for (i = 0; i < 1024; i++) {
		/* !W */
		pgtbl[i] = (lma + (i << LOG_PGSIZE)) | (7 & ~2);
	}

	rrupts_off();

	set_trap_gate(PF_VECTOR, __GUEST_CS, (uval32)cow_handler, 0xce00);

	i = hcall_page_dir(ret, H_PAGE_DIR_ACTIVATE, (uval)pgdir);
	assert(i == H_Success, "hcall_page_dir failed\n");

	rrupts_on();

	/* start accessing from 4M, 8M */
	hprintf("Start accessing ...\n");
	ptr1 = (uval *)new_vma;
	ptr2 = (uval *)old_vma;
	for (i = 0; i < (0x400000 / sizeof (uval)); i += 0x1000) {
		assert((pgtbl[(((int)&ptr1[i]) >> LOG_PGSIZE) & 0x3FF] &
			(1 << 6)) == 0, "dirty set");

		ptr1[i] = i;	/* cause CoW */

		assert((pgtbl[(((int)&ptr1[i]) >> LOG_PGSIZE) & 0x3FF] &
			(1 << 6)) != 0, "dirty not set");

		j = ptr2[i];
		if (j != i) {
			hprintf("i = 0x%lx (0x%lx), MISMATCHES "
				"ptr1[i] = 0x%lx, ptr2[i] = 0x%lx\n",
				i, j, ptr1[i], ptr2[i]);
		} else {
			hprintf("i = 0x%lx matches\n", i);
		}

	}
	hprintf("Done, returning ...");

	return 0;
}
