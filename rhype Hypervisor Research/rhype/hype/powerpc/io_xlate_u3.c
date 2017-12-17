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

#include <types.h>
#include <hype.h>
#include <lib.h>
#include <pgalloc.h>
#include <io_xlate.h>
#include <hba.h>
#include <pmm.h>
#include <io.h>

#define DART_VALID	0x80000000
#define DART_MASK	0x00ffffff
#define DART_BASE       0xf8033000UL
#define LOG_DARTSZ	21

union dart_ctl {
	struct {
		uval32	dc_base:20;
		uval32	__dc_unknown:1;
		uval32	dc_invtlb:1;
		uval32	dc_enable:1;
		uval32	dc_size:9;
	} reg;
	uval32 word;
};


static uval32 *dart_table;
static void *dummy_page;
static uval32 *dart_ctl_reg;

static void
fill_dart(uval index, uval raddr, uval num_pg)
{
	uval32 *entry = dart_table + index;
	uval i = 0;
	uval rpg = raddr >> LOG_PGSIZE;
	uval last_flush = 0;
	while (1) {
		entry[i] = DART_VALID | (rpg & DART_MASK);
		++i;
		++rpg;
		if (i == num_pg) break;

		if (((uval)&entry[i]) % CACHE_LINE_SIZE == 0) {
			last_flush = (uval) &entry[i - 1];
			dcbst(last_flush);
		}
	}
	dcbst((uval) &entry[i - 1]);

}

static void
clear_dart(uval index, uval num_pg)
{
	uval32 *entry = dart_table + index;
	uval i = 0;
	uval rpg = (uval)dummy_page;
	uval last_flush = 0;
	while (1) {
		entry[i] = DART_VALID | (rpg & DART_MASK);
		++i;
		if (i == num_pg) break;

		if (((uval)&entry[i]) % CACHE_LINE_SIZE == 0) {
			last_flush = (uval) &entry[i - 1];
			dcbst(last_flush);
		}
	}
	dcbst((uval) &entry[i - 1]);
}

static void
invtlb_dart(void)
{
	union dart_ctl dc;
	dc.word = io_in32(dart_ctl_reg);
	dc.reg.dc_invtlb = 1;

	io_out32(dart_ctl_reg, dc.word);

	union dart_ctl copy;
	uval retries = 0;
	do {
		copy.word = io_in32(dart_ctl_reg);
		if (copy.reg.dc_invtlb == 0) {
			break;
		}

		++retries;
		if ((retries & 3) == 0) {
			copy.reg.dc_invtlb = 0;
			io_out32(dart_ctl_reg, copy.word);
			copy.word = io_in32(dart_ctl_reg);
			io_out32(dart_ctl_reg, dc.word);
		}
	} while (1);
}

uval
io_xlate_init(uval ofd)
{
	(void)ofd;

	/* SLOF doesn't provide DART address */
	dart_ctl_reg = (uval32*)DART_BASE;

	/* Max size of 512 pages == 2MB == 1<<21 */
	dart_table = (uval32*)get_pages_aligned(&phys_pa,
						1<<LOG_DARTSZ, LOG_DARTSZ);


	/* Linux uses a dummy page, filling "empty" DART netries with
	   a reference to this page to capture stray DMA's */
	dummy_page = (void*)get_pages(&phys_pa, PGSIZE);

	clear_dart(0, (1 << LOG_DARTSZ) / sizeof(uval32));

	union dart_ctl dc;
	dc.reg.dc_base = ((uval)dart_table) >> LOG_PGSIZE;
	dc.reg.dc_size = 1 << (LOG_DARTSZ - LOG_PGSIZE);
	dc.reg.dc_enable = 1;


	hprintf("Initializing U3 DART: tbl: %p ctl reg: %p word: %x\n",
		dart_table, dart_ctl_reg, dc.word);

	io_out32(dart_ctl_reg, dc.word);

	invtlb_dart();

	return 0;
}

sval
io_xlate_put(struct os *os, uval32 buid, uval ioba, union tce ltce)
{
	struct hba *hba = hba_get(buid);
	uval index = ioba >> LOG_PGSIZE;
	uval la;
	uval pa;

	la = ltce.tce_bits.tce_rpn << LOG_PGSIZE;
	pa = logical_to_physical_address(os, la, PGSIZE);
	if (!hba || !hba_owner(hba, os->po_lpid)) {
		return H_Parameter;
	}
	if (pa == INVALID_PHYSICAL_ADDRESS) {
		return H_Parameter;
	}

	if (ltce.tce_bits.tce_read || ltce.tce_bits.tce_write) {
		fill_dart(index, pa, 1);
	} else {
		clear_dart(index, 1);
	}
	invtlb_dart();

	return H_Success;
}
