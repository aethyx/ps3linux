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
 * Routines to read and write bytes in memory mapped space.
 *
 */

#include <config.h>
#include <hv_asm.h>


### In order to access device registers (and not just look at the cache)
### the address space must be accessed with caching inhibited.  Use BAT 0
### for this purpose.  Just in case it was defined for something else,
### save its contents in r8 and r9, and then restore them before exiting.
### 
### void io_out8(address, value);
### 
### 3 value = io_in8(address);
### 
### WARNING:  NOT SMP SAFE, Interrupts may also cause problems.

ACCESSOR_F(io_out8, r5, r6, stb r4, 0(r3))
ACCESSOR_F(io_in8, r5, r6, lbz r3, 0(r3))
ACCESSOR_F(io_out16, r5, r6, sth r4, 0(r3))
ACCESSOR_F(io_in16, r5, r6, lhz r3, 0(r3))
ACCESSOR_F(io_out32, r5, r6, stw r4, 0(r3))
ACCESSOR_F(io_in32, r5, r6, lwz r3, 0(r3))
ACCESSOR_F(io_out64, r5, r6, std r4, 0(r3))
ACCESSOR_F(io_in64, r5, r6, ld r3, 0(r3))

ACCESSOR_F(io_out16LE, r5, r6, sthbrx r4, 0, r3)
ACCESSOR_F(io_in16LE, r5, r6, lhbrx r3, 0, r3)
ACCESSOR_F(io_out32LE, r5, r6, stwbrx r4, 0, r3)
ACCESSOR_F(io_in32LE, r5, r6, lwbrx r3, 0, r3)


C_TEXT_ENTRY(io_out_sz)
	cmpwi	r0, r5, 2
	beq	r0, .io_out16
	bgt	1f
	b	.io_out8
1:	
	cmpwi	r0, r5, 4
	beq	r0, .io_out32
	b	.io_out64
C_TEXT_END(io_out_sz)
				
C_TEXT_ENTRY(io_in_sz)
	cmpwi	r0, r4, 2
	beq	r0, .io_in16
	bgt	1f
	b	.io_in8
1:	
	cmpwi	r0, r4, 4
	beq	r0, .io_in32
	b	.io_in64
C_TEXT_END(io_in_sz)
				
C_TEXT_ENTRY(tick)
1:
	ACCESSOR(r7, r8, lbz r9,0(r3))
        andi.	r9,r9,4
        beq+	0, 1b
	eieio			
	ACCESSOR(r7,r8, stb r4,16(r3))
	blr			
C_TEXT_END(tick)
