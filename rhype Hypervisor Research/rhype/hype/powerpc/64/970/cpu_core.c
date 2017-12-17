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
#include <asm.h>
#include <cpu.h>
#include <lib.h>
#include <hid.h>
#include <lpidtag.h>
#include <machine_config.h>

void
cpu_core_init(uval ofd, uval cpuno, uval cpuseq)
{
	(void)ofd;
	(void)cpuno;

	uval old0, new0;
	uval old1, new1;
	uval old4, new4;
	uval old5, new5;

	__asm__ __volatile__("isync; slbia; isync" : : : "memory");

	union hid0 hid0 = { .word = get_hid0() };
	old0 = hid0.word;
	hid0.bits.nap = 1;
	hid0.bits.dpm = 1;
	hid0.bits.nhr = 1;
	hid0.bits.ext_tb_enb = 0;
	hid0.bits.hdice = 1;
	hid0.bits.eb_therm = 1;
	hid0.bits.en_attn = 1;

	new0 = hid0.word;
	set_hid0(hid0.word);


	union hid1 hid1 = { .word = get_hid1() };
	old1 = hid1.word;

	hid1.bits.bht_pm = 7;
	hid1.bits.en_ls = 1;

	hid1.bits.en_cc = 1;
	hid1.bits.en_ic = 1;

	hid1.bits.pf_mode = 2;

	hid1.bits.en_if_cach = 1;
	hid1.bits.en_ic_rec = 1;
	hid1.bits.en_id_rec = 1;
	hid1.bits.en_er_rec = 1;

	hid1.bits.en_sp_itw = 1;

	new1 = hid1.word;
	set_hid1(hid1.word);

	union hid4 hid4 = { .word = get_hid4() };
	old4 = hid4.word;
	hid4.bits.lpes0	= 0;
	hid4.bits.lpes1 = 1;

	new4 = hid4.word;
	set_hid4(hid4.word);

	union hid5 hid5 = { .word = get_hid5() };
	old5 = hid5.word;
	hid5.bits.DCBZ_size = 0;
	hid5.bits.DCBZ32_ill = 0;

	new5 = hid5.word;
	set_hid5(hid5.word);
	__asm__ __volatile__("isync; slbia; isync" : : : "memory");

	hprintf("hid0: 0x%016lx -> 0x%016lx\n", old0, new0);
	hprintf("hid1: 0x%016lx -> 0x%016lx\n", old1, new1);
	hprintf("hid4: 0x%016lx -> 0x%016lx\n", old4, new4);
	hprintf("hid5: 0x%016lx -> 0x%016lx\n", old5, new5);

	if (cpuseq == 0) {
		lpidtag_init(NUM_LPIDR_BITS);
	}
}
