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
#include <config.h>
#include <asm.h>
#include <lpar.h>
#include <hypervisor.h>

/* in is unsused */
#define HCALL(in, out, name, func_code)	\
	C_TEXT_ENTRY(name); 		\
	STR r3,-REG_WIDTH(r1); 		\
	li r3,func_code; 		\
	HSC;				\
	LDR r12,-REG_WIDTH(r1);		\
	cmpi 0,r12,0;			\
        bne ret	## out;	/* only store regs if r12 != NULL */ \
	b ret0

ret8:	STR r11, 7 * REG_WIDTH(r12)
ret7:	STR r10, 6 * REG_WIDTH(r12)
ret6:	STR r9, 5 * REG_WIDTH(r12)
ret5:	STR r8, 4 * REG_WIDTH(r12)
ret4:	STR r7, 3 * REG_WIDTH(r12)
ret3:	STR r6, 2 * REG_WIDTH(r12)
ret2:	STR r5, 1 * REG_WIDTH(r12)
ret1:	STR r4, 0 * REG_WIDTH(r12)
	nop
ret0:	blr
