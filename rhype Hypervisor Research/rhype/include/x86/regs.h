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
 * CPU specific definitions
 */
#ifndef __X86_REGS_H__
#define __X86_REGS_H__

#define CR0_PE		(1 << 0)
#define CR0_MP		(1 << 1)
#define CR0_EM		(1 << 2)
#define CR0_TS		(1 << 3)
#define CR0_ET		(1 << 4)
#define CR0_NE		(1 << 5)
#define CR0_WP		(1 << 16)
#define CR0_AM		(1 << 18)
#define CR0_NW		(1 << 29)
#define CR0_CD		(1 << 30)
#define CR0_PG		(1 << 31)

#define CR4_VME		(1 << 0)
#define CR4_PVI		(1 << 1)
#define CR4_TSD		(1 << 2)
#define CR4_DE		(1 << 3)
#define CR4_PSE		(1 << 4)
#define CR4_PAE		(1 << 5)
#define CR4_MCE		(1 << 6)
#define CR4_PGE		(1 << 7)
#define CR4_PCE		(1 << 8)
#define CR4_OSFXSR	(1 << 9)
#define CR4_OSXMMEXCPT	(1 << 10)

#define EFLAGS_CF	(1 << 0)
#define EFLAGS_PF	(1 << 2)
#define EFLAGS_AF	(1 << 4)
#define EFLAGS_FF	(1 << 6)
#define EFLAGS_SF	(1 << 7)
#define EFLAGS_TF	(1 << 8)
#define EFLAGS_IF	(1 << 9)
#define EFLAGS_DF	(1 << 10)
#define EFLAGS_OF	(1 << 11)
#define EFLAGS_NT	(1 << 14)
#define EFLAGS_RF	(1 << 16)
#define EFLAGS_VM	(1 << 17)
#define EFLAGS_AC	(1 << 18)
#define EFLAGS_VIF	(1 << 19)
#define EFLAGS_VIP	(1 << 20)
#define EFLAGS_ID	(1 << 21)

#define EFLAGS_IOPL_0	(0x0 << 12)
#define EFLAGS_IOPL_1	(0x1 << 12)
#define EFLAGS_IOPL_2	(0x2 << 12)
#define EFLAGS_IOPL_3	(0x3 << 12)
#define EFLAGS_IOPL_MASK	(0x3 << 12)

#define	CPUID_FEATURES_PSE	0x0000008	/* bit  3 */
#define	CPUID_FEATURES_CX8	0x0000100	/* bit  8 */
#define	CPUID_FEATURES_PGE	0x0002000	/* bit 13 */
#define CPUID_FEATURES_FXSAVE	0x1000000	/* bit 24 */
#define	CPUID_FEATURES_XMM	0x2000000	/* bit 25 */

#ifndef __ASSEMBLY__

struct descriptor {
	uval32 word0;
	uval32 word1;
};

static inline uval
get_cr0(void)
{
	uval ret;

	/* *INDENT-OFF* */
	__asm__ __volatile__(
			"movl	%%cr0, %0"
			: "=r"(ret)
	);
	/* *INDENT-ON* */

	return ret;
}

static inline void
set_cr0(uval value)
{
	/* *INDENT-OFF* */
	__asm__ __volatile__(
			"movl	%0, %%cr0\n"
			"jmp	1f\n"
			"1: 	nop\n"
			: /* no outputs */
			: "r"(value)
	);
	/* *INDENT-ON* */
}

static inline uval
get_cr2(void)
{
	uval ret;

	/* *INDENT-OFF* */
	__asm__ __volatile__(
			"movl	%%cr2, %0"
			: "=r"(ret)
	);
	/* *INDENT-ON* */

	return ret;
}

static inline uval
get_cr3(void)
{
	uval ret;

	/* *INDENT-OFF* */
	__asm__ __volatile__(
			"movl	%%cr3, %0"
			: "=r"(ret)
	);
	/* *INDENT-ON* */

	return ret;
}

static inline void
set_cr3(uval addr)
{
	/* *INDENT-OFF* */
	__asm__ __volatile__(
			"movl	%0, %%cr3"
			: /* no outputs */
			: "r"(addr)
	);
	/* *INDENT-ON* */
}

#endif /* __ASSEMBLY__ */

#endif /* __X86_REGS_H__ */
