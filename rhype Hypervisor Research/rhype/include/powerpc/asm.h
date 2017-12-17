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
 *
 */
#ifndef _POWERPC_ASM_H
#define _POWERPC_ASM_H

#include <util.h>
#include <asmdef.h>
#include <msr.h>

#ifdef HAS_HYPE_SYSCALL
#define HSC	.long 0x44000022
#else
#define HSC	li r0,-1; sc
#endif

#ifndef __ASSEMBLY__
#include <types.h>

static __inline__ uval32
mfpvr(void)
{
	uval32 pvr;
	__asm__ __volatile__ ("mfpvr	%0" : "=&r" (pvr));
	return pvr;
}

static __inline__ uval
mfmsr(void)
{
	uval msr;
	__asm__ __volatile__("mfmsr	%0":"=&r"(msr));

	return msr;
}

static __inline__ void mtmsr(uval msr) {
	__asm__ __volatile__ ("mtmsr	%0"
			      : /* output */
			      : "r" (msr));
}

static __inline__ void
hw_dcache_flush(uval addr)
{
	__asm__ __volatile__(
		"dcbst 0,%0    \n"	/* flush this data cache line */
		"sync          \n"	/* force the dcbst to complete */
		/* invalidate any prior icache prefetching */
		"icbi 0,%0     \n"	
		: /* no outputs */
		: "r" (addr) );
}

static __inline__ uval
mfsdar(void)
{
	uval ret;
	__asm__ __volatile__ ("mfspr %0,%1": "=&r" (ret) : "i" (SPRN_SDAR));
	return ret;
}
static __inline__ uval
mfsiar(void)
{
	uval ret;
	__asm__ __volatile__ ("mfspr %0,%1": "=&r" (ret) : "i" (SPRN_SIAR));
	return ret;
}
static __inline__ uval
mfdar(void)
{
	uval ret;
	__asm__ __volatile__ ("mfspr %0,%1": "=&r" (ret) : "i" (SPRN_DAR));
	return ret;
}

static __inline__ uval
mfdsisr(void)
{
	uval ret;
	__asm__ __volatile__ ("mfspr %0,%1": "=&r" (ret) : "i" (SPRN_DSISR));
	return ret;
}

static __inline__ uval
mfsrr0(void)
{
	uval ret;
	__asm__ __volatile__ ("mfspr %0,%1": "=&r" (ret) : "i" (SPRN_SRR0));
	return ret;
}

static __inline__ uval
mfsrr1(void)
{
	uval ret;
	__asm__ __volatile__ ("mfspr %0,%1": "=&r" (ret) : "i" (SPRN_SRR1));
	return ret;
}

static __inline__ uval32
mfdec(void)
{
	uval32 ret;
	__asm__ __volatile__ ("mfspr %0,%1": "=&r" (ret) : "i" (SPRN_DEC));
	return ret;
}

static __inline__ uval32
mfpir(void)
{
	uval32 ret;
	__asm__ __volatile__("mfspr %0,%1" : "=&r" (ret) : "i" (SPRN_PIR));
	return ret;
}

static __inline__ void
mtsrr0(uval val)
{
	__asm__ __volatile__("mtspr %0,%1"::"i"(SPRN_SRR0), "r"(val));
}

static __inline__ void
mtsrr1(uval val)
{
	__asm__ __volatile__("mtspr %0,%1"::"i"(SPRN_SRR1), "r"(val));
}

static __inline__ void
mtdar(uval val)
{
	__asm__ __volatile__("mtspr %0,%1"::"i"(SPRN_DAR), "r"(val));
}

static __inline__ void
mtdsisr(uval val)
{
	__asm__ __volatile__("mtspr %0,%1"::"i"(SPRN_DSISR), "r"(val));
}

static __inline__ void
mtdec(uval32 val)
{
	__asm__ __volatile__("mtspr %0,%1"::"i"(SPRN_DEC), "r"(val));
}

#endif /* !__ASSEMBLY__ */
#endif /* _POWERPC_ASM_H */
