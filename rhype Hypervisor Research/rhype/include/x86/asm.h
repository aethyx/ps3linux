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
#ifndef _X86_ASM_H_
#define _X86_ASM_H_

#include <config.h>
#include <util.h>

/* defined for both asm and C */
#define REG_WIDTH       4
#define MIN_FRAME_SZ    0

/* segment selectors in GDT */
#define __HV_CS		0x8
#define __HV_DS		0x10
#define __TSS		0x18
#define __GUEST_CS	0x22
#define __GUEST_DS	0x2a

#ifdef __ASSEMBLY__
/* Assembler only */

#define DOT .
#define TEXT_ENTRY(x) \
	 .section ".text"; .align 2; GLBL_LABEL(x)
#define TEXT_END(x) \
	 .size	x,DOT-x
#define DATA_ENTRY(x) \
	.section ".data"; .align x;

#define GLBL_LABEL(x) .globl x; x:
#define C_TEXT(x) x
#define C_DATA(x) x

#define C_TEXT_ENTRY(x) \
	TEXT_ENTRY(C_TEXT(x))

#define C_TEXT_END(x) \
        TEXT_END(C_TEXT(x))

#define C_DATA_ENTRY(x) \
	DATA_ENTRY(3) \
	GLBL_LABEL(C_DATA(x))

#define CALL_CFUNC(rn)		/* XXX write me */
#define LOADADDR(rn, name)	leal	name, rn
#define LOADCONST(rn, c)	movl	$c, rn

#define HCALL_VEC_IDX_SHIFT	1
#define HCALL_VEC_MASK_NUM	(29-HCALL_VEC_IDX_SHIFT)

#endif /* __ASSEMBLY__ */

#endif /* ! _X86_ASM_H_ */
