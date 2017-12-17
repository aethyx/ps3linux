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
#include <lib.h>
#include <thinwire.h>
#include <sim.h>
#include "tlb_4xx.h"

/* XXX if possible (?), this needs to be cleaned up */
void
hypervisorInit(int on_hardware)
{
	struct io_chan *serial;
	uval baudrate;

#warning "check that baudrate is correct"
	/* this was (691200/115200) = 6 */
	baudrate = 691200 / BAUDRATE;	/* ??? */

	if (on_hardware) {
		int i;

		/* clear out leftover TLBEs (except for the ones we like) */
		for (i = MIN_OS_TLBX; i < 16; i++) {
			tlbwe(i, 0, 0, 0);
		}

		serial = uartNS16750_init(UART0Effective, 1, baudrate);
#ifdef USE_THINWIRE_IO		/* thinwire serial IO */
		serial_init_fn = uartNS16750_init;
		thinwireInit(serial);
		bprintInit(&thinwireMux, 1);
		h_term_init(&thinwireMux, 4);
#else  /* raw serial IO */
		interleaveInit(serial);
		bprintInit(&interleaveMux, 1);
		h_term_init(&interleaveMux, 0);
#endif

	} else {
		serial_init_fn = uartNS16750_init;
		serial = uartNS16750_init(UART0Effective, 0, baudrate);

		console = interleaveInit(serial);

		bprintInit(console, 1);
		hype_term_init(console, 0);
	}
}

/* hardware routines */

/* hw_switch_tlb()
 * Starting with #3, copy TLBEs from new_pcop->utlb into the hw UTLB.
 * No need to save the old TLBEs off, that is done in hyp_enter_4xx().
 *
 *	new_pcop - pointer to per_cpu_os being switched in
 */
static void
hw_switch_tlb(struct cpu *new_pcop)
{
	int i;

	/* write TLBEs 3 to NUM_TLBENTRIES to the hw UTLB */
	for (i = 3; i < NUM_TLBENTRIES; i++) {
		/* zero STID field */
		uval old_mmucr = get_mmucr() & ~MMUCR_STID_MASK;

		/* first write the new STID to hw MMUCR */
		set_mmucr(old_mmucr | new_pcop->utlb[i].bits.tid);

		/* now write the new TLBE, overwriting whatever was there */
		tlbwe(i, new_pcop->utlb[i].words.epnWord,
		      new_pcop->utlb[i].words.rpnWord,
		      new_pcop->utlb[i].words.attribWord);
	}
}
