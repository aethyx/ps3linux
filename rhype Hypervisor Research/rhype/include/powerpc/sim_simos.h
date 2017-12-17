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

#ifndef __SIM_SIMOS_H_
#define __SIM_SIMOS_H_

#include <asm.h>

#define SIM_REAL_MODE 0
#define SIM_XLATE_MODE 1

#define SIM_WRITE	0x00	/* write to console */
#define SIM_TW_READ	0x3d	/* thinwire */
#define SIM_TW_WRITE	0x3e
#define SIM_MEMCPY	0x45
#define SIM_MEMSET	0x47

#define MSR_SIM		ULL(0x0000000020000000)
#define MSR_MAMBO	ULL(0x0000000010000000)

#ifdef __ASSEMBLY__

/* Use reg to decide to call sim or not */
#define ONSIM(reg, sim) \
	mfmsr	reg; \
	andis.	reg, reg, ((MSR_SIM | MSR_MAMBO) >> 16); \
	bne	0, sim

#define OFFSIM(reg, hw) \
	mfmsr	reg; \
	andis.	reg, reg, ((MSR_SIM | MSR_MAMBO) >> 16); \
	beq	0, hw

#define OFFSIM_XLATE(reg1, reg2, hw) \
	mfmsr	reg1; \
	andis.	reg2, reg1, ((MSR_SIM | MSR_MAMBO) >> 16); \
	beq	0, hw; \
	andi.   reg2, reg1, (MSR_IR | MSR_DR); \
	bne	0, hw

#define SIMOS_BREAKPOINT   .long 0x7c0007ce

#else  /* __ASSEMBLY__ */

#include <config.h>
#include <types.h>

#define SIMOS_BREAKPOINT   asm(".long 0x7c0007ce")

extern uval SimOSSupport(unsigned int foo, ...);

/*
 * support routines -- using special features of SimOS and Mambo
 */

extern void *sim_memcpy(void *dst, const void *src, uval size);
extern void *sim_memset(void *dst, int value, uval size);
extern sval sim_write(const char *lbuf, uval len);
extern sval sim_tw_write(const char *buf, uval length);
extern sval sim_tw_read(char *buf, uval length);

#endif /* ! __ASSEMBLY__ */

#endif /* ! __SIM_H_ */
