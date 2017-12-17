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

#ifndef _POWERPC_TEST_H
#define _POWERPC_TEST_H

#include_next <test/test.h>
#include <msr.h>
#include <openfirmware.h>

extern void custom_init(uval r3, uval r4, uval ofh, uval r6)
	__attribute__ ((weak));

static inline void
rrupts_on(void) {
	uval msr;
	__asm__ __volatile__ (
		"mfmsr	%0		\n\t"
		"ori	%0,%0,%1	\n\t"
		"mtmsr	%0"
		: "=&r" (msr) : "i" (MSR_EE));
}
static inline void
rrupts_off(void) {
	uval msr;
	__asm__ __volatile__ (
		"mfmsr	%0		\n\t"
		"andc	%0,%0,%1	\n\t"
		"mtmsr	%0"
		: "=&r" (msr) : "r" (MSR_EE));
}

static __inline__ void prom_init(uval addr, uval of_msr) {
#ifdef USE_OPENFIRMWARE
	hprintf("assigning OF Handler to 0x%lx\n", addr);
	of_init(addr, of_msr);
#endif
}

extern uval prom_load(struct partition_status* ps, uval ofd);
extern void crt_init(uval r3, uval r4, uval r5, uval r6);
#endif	/* _POWERPC_TEST_H */
