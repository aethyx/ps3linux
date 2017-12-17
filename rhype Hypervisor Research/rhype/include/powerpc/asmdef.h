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
#ifndef _ASMDEF_H
#define _ASMDEF_H

#include <config.h>

#ifdef __ASSEMBLY__

/* General Purpose Registers (GPRs) */

#define	r0	0
#define	r1	1
#define	r2	2
#define	r3	3
#define	r4	4
#define	r5	5
#define	r6	6
#define	r7	7
#define	r8	8
#define	r9	9
#define	r10	10
#define	r11	11
#define	r12	12
#define	r13	13
#define	r14	14
#define	r15	15
#define	r16	16
#define	r17	17
#define	r18	18
#define	r19	19
#define	r20	20
#define	r21	21
#define	r22	22
#define	r23	23
#define	r24	24
#define	r25	25
#define	r26	26
#define	r27	27
#define	r28	28
#define	r29	29
#define	r30	30
#define	r31	31

/* Floating Point Registers (FPRs) */

#define	f0	0
#define	f1	1
#define	f2	2
#define	f3	3
#define	f4	4
#define	f5	5
#define	f6	6
#define	f7	7
#define	f8	8
#define	f9	9
#define	f10	10
#define	f11	11
#define	f12	12
#define	f13	13
#define	f14	14
#define	f15	15
#define	f16	16
#define	f17	17
#define	f18	18
#define	f19	19
#define	f20	20
#define	f21	21
#define	f22	22
#define	f23	23
#define	f24	24
#define	f25	25
#define	f26	26
#define	f27	27
#define	f28	28
#define	f29	29
#define	f30	30
#define	f31	31

/* Condition Register Bit Fields */
#define	cr0	0
#define	cr1	1
#define	cr2	2
#define	cr3	3
#define	cr4	4
#define	cr5	5
#define	cr6	6
#define	cr7	7

/* bits within a CR field, for conditional branch tests */
#define	lt	0
#define	gt	1
#define	eq	2
#define	so	3

/* stack pointer */
#define	sp	1

#define	sprg0	0
#define	sprg1	1
#define	sprg2	2
#define	sprg3	3

/* vmx registers */
#define vr0	0
#define vr1	1
#define vr2	2
#define vr3	3
#define vr4	4
#define vr5	5
#define vr6	6
#define vr7	7
#define vr8	8
#define vr9	9
#define vr10	10
#define vr11	11
#define vr12	12
#define vr13	13
#define vr14	14
#define vr15	15
#define vr16	16
#define vr17	17
#define vr18	18
#define vr19	19
#define vr20	20
#define vr21	21
#define vr22	22
#define vr23	23
#define vr24	24
#define vr25	25
#define vr26	26
#define vr27	27
#define vr28	28
#define vr29	29
#define vr30	30
#define vr31	31
#define vrsave	256

#endif /* __ASSEMBLY__ */
#endif /* ! _ASMDEF_H */
