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
#ifndef _POWERPC_64_HV_ASM_H
#define _POWERPC_64_HV_ASM_H

#include <asm.h>
#include_next <hv_asm.h>
#include <hv_regs.h>

#ifdef __ASSEMBLY__

/*
 * get/put the HSRR* registers (or SRR* registers) as appropriate
 */
#ifdef HAS_MEDIATED_EE
/*
 * if LPES[0] is not 0 the get the values from srr0 and srr1
 */
#define GET_SRR(num,rs)		mfspr rs,SPRN_HSRR##num
#define PUT_SRR(num, reg)	mtspr SPRN_HSRR##num, reg
#else
#define GET_SRR(num,rs)		mfsrr##num rs
#define PUT_SRR(num, reg)	mtsrr##num, reg
#endif
/*
 * Cache Inhibited Load and Store
 */
#define CI_GET_SPR(reg) mfspr reg, SPRN_HID4
/* some processors need special attention when setting some HIDs */
#define CI_SET_SPR(r) SET_HID4(r)

/* Most common case has the bit in HID4[23] and you set to turn on*/
#define CI_SET(rci, rsave) \
	lis   rci, 0x100; \
	sldi  rci, rci, 16; \
	or    rci, rci, rsave

#define CI_ENABLE(rsave, rci)	\
	CI_GET_SPR(rsave);	\
	CI_SET(rci, rsave); \
	CI_SET_SPR(rci)

#define CI_DISABLE(rsave) CI_SET_SPR(rsave)

/*
 * We set the EA[0]=1, this way if the HRMOR is non-zero then is will
 * be ignored.  If the processor does NOT support HRMOR then
 * architecurally this has no effect.  If you really do not want to
 * run these 3 piddley insns then undef it in you architecture.
 * Assumes address in r3.
 */
#define CI_SET_EA0(rs) li rs, -1; rldicr rs, rs, 0, 0; or r3, r3, rs

#define ACCESSOR(rsave, rci , ARGS...)	\
	CI_SET_EA0(rsave);	\
	CI_ENABLE(rsave, rci);	\
	ARGS;			\
	eieio;			\
	CI_DISABLE(rsave)

#define ACCESSOR_F(name, rsave, rci , ARGS...)	\
C_TEXT_ENTRY(name)		\
	ACCESSOR(rsave, rci, ARGS); \
	blr;			\
C_TEXT_END(name)

#else  /* __ASSEMBLY__ */

#ifndef ASM_PTESYNC
#define ASM_PTESYNC __asm__ __volatile__("ptesync")
#endif
#ifndef ASM_TLBSYNC
#define ASM_TLBSYNC __asm__ __volatile__("tlbsync")
#endif
/* These are not implemented in pu */
static __inline__ void
ptesync(void)
{
	ASM_PTESYNC;
}

static __inline__ void
tlbsync(void)
{
	ASM_TLBSYNC;
}

#endif /* __ASSEMBLY__ */

#endif /* _POWERPC_64_HV_ASM_H */
