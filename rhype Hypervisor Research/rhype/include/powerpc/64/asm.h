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
#ifndef _POWERPC_64_ASM_H
#define _POWERPC_64_ASM_H

#include <regs.h>
#include <include/powerpc/asm.h>

/*
 * Special instruction sequence for HID access (according to GR book 4)
 */
#define SET_HID0(Rx)	sync ; mtspr SPRN_HID0,Rx ; \
			mfspr Rx,SPRN_HID0 ; mfspr Rx,SPRN_HID0 ; \
			mfspr Rx,SPRN_HID0 ; mfspr Rx,SPRN_HID0 ; \
			mfspr Rx,SPRN_HID0 ; mfspr Rx,SPRN_HID0 ; \
			isync

#define SET_HID1(Rx)	mtspr SPRNHID1,Rx ; mtspr SPRNHID1,Rx ; isync
#define SET_HID4(Rx)	sync ; mtspr SPRN_HID4,Rx ; isync

#define LOADCONST(rn,c) \
	lis	rn,((c) >> 48);			\
	ori	rn,rn,(((c) >> 32) & 0xffff);	\
	rldicr	rn,rn,32,31;			\
	oris	rn,rn,(((c) >> 16) & 0xffff);	\
	ori	rn,rn,((c) & 0xffff)

#define LOADADDR(rn,name) \
	lis	rn,name##@highest;	\
	ori	rn,rn,name##@higher;	\
	rldicr	rn,rn,32,31;		\
	oris	rn,rn,name##@h;		\
	ori	rn,rn,name##@l

#define DOT .
#define FUNCDESC(x,f) \
	.section ".opd","aw"; .align 3; \
	GLBL_LABEL(x) .quad f,.TOC.@tocbase, 0; .previous; .size x,24;
#define TEXT_ENTRY(x) \
	 .section ".text"; .align 2; .type x,@function; GLBL_LABEL(x)
#define TEXT_END(x) \
	 .size	x,DOT-x
#define DATA_ENTRY(x) \
	.section ".data"; .align x;

#define GLBL_LABEL(x) .globl x; x:
#define C_TEXT(x) . ## x
#define C_DATA(x) x

#define C_TEXT_ENTRY(x) \
	FUNCDESC(x, C_TEXT(x)) \
	TEXT_ENTRY(C_TEXT(x))

#define C_TEXT_END(x) \
        TEXT_END(C_TEXT(x))

#define C_DATA_ENTRY(x) \
	DATA_ENTRY(3) \
	GLBL_LABEL(C_DATA(x))

#define CALL_FUNCDESC(rn) /* call through a function descriptor */ \
	LDR	r2, 8(rn); /* load function's TOC value */ \
	LDR	rn, 0(rn); \
	mtctr	rn; \
	bctrl ;     \
        nop

#define CALL_CFUNC	CALL_FUNCDESC

/* 64-bit load/store */
#define LDR ld
#define STR std
/* 64-bit load/store update*/
#define LDRU ldu
#define STRU stdu

#define	CMPI	cmpdi
#define CMPL	cmpld
#define MTMSR	mtmsrd

#define RFI	rfid

#define HCALL_VEC_IDX_SHIFT	1
#define HCALL_VEC_MASK_NUM	(61-HCALL_VEC_IDX_SHIFT)
#define RLICR	rldicr
#define ATTN	.long 0x00000200

#ifndef __ASSEMBLY__
#include <types.h>

/*
 * move from SPR
 */

static __inline__ uval
mfsprg0(void)
{
	uval ret;
	__asm__ __volatile__ ("mfspr %0,%1": "=&r" (ret) : "i" (SPRN_SPRG0));
	return ret;
}

static __inline__ uval
mfsprg1(void)
{
	uval ret;
	__asm__ __volatile__ ("mfspr %0,%1": "=&r" (ret) : "i" (SPRN_SPRG1));
	return ret;
}

static __inline__ uval
mfsprg2(void)
{
	uval ret;
	__asm__ __volatile__ ("mfspr %0,%1": "=&r" (ret) : "i" (SPRN_SPRG2));
	return ret;
}

static __inline__ uval
mfsprg3(void)
{
	uval ret;
	__asm__ __volatile__ ("mfspr %0,%1": "=&r" (ret) : "i" (SPRN_SPRG3));
	return ret;
}

static __inline__ uval
mftb(void)
{
	uval ret;
	__asm__ __volatile__ ("mftb %0": "=&r" (ret) : /* inputs */);
	return ret;
}

/*
 * move to SPR
 */

static __inline__ void
mtsprg0(uval val)
{
	__asm__ __volatile__("mtspr %0,%1"::"i"(SPRN_SPRG0), "r"(val));
}

static __inline__ void
mtsprg1(uval val)
{
	__asm__ __volatile__("mtspr %0,%1"::"i"(SPRN_SPRG1), "r"(val));
}

static __inline__ void
mtsprg2(uval val)
{
	__asm__ __volatile__("mtspr %0,%1"::"i"(SPRN_SPRG2), "r"(val));
}

static __inline__ void
mtsprg3(uval val)
{
	__asm__ __volatile__("mtspr %0,%1"::"i"(SPRN_SPRG3), "r"(val));
}

/*
 * misc
 */

static __inline__ void
slbia(void)
{
	__asm__ __volatile__("isync; slbia; isync":::"memory");
}

static __inline__ void
slbie(uval entry)
{
	/* *INDENT-OFF* */
	__asm__ __volatile__(
		"isync		\n\t"
		"slbie %0	# invalidat SLB[0]\n\t"
		"isync		\n"
		: : "r" (entry) : "memory");
	/* *INDENT-ON* */
}

static __inline__ int
cntlzw(uval32 val)
{
	int ret;
	__asm__ __volatile__("cntlzw %0,%1" :"=&r" (ret) : "r" (val));
	return ret;
}

static __inline__ void
icbi(uval line)
{
	__asm__ __volatile__("icbi 0, %0"::"r"(line):"memory");
}

static __inline__ void
dcbf(uval addr)
{
	__asm__ __volatile__("dcbf 0, %0"::"r"(addr):"memory");
}

static __inline__ uval
mfasr(void)
{
	uval ret;
	__asm__ __volatile__("mfasr %0":"=r"(ret));

	return ret;
}

static __inline__ void
mtasr(uval val)
{
	__asm__ __volatile__("mtasr %0"::"r"(val));
}

static __inline__ uval
get_toc(void)
{
	uval toc = 0;
	__asm__ __volatile__ ("mr %0,2" : "=r" (toc));
	return toc;
}

static __inline__ uval
get_sp(void)
{
	uval sp = 0;
	__asm__ __volatile__ ("mr %0,1" : "=r" (sp));
	return sp;
}

#endif /* ! __ASSEMBLY__ */
#endif /* _POWERPC_64_ASM_H */
