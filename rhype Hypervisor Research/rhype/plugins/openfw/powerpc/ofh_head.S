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
 * Glue code for open-firmware client interface implementation.
 *
 */

/*
 * This is very similar to ofh_head.S. will merge later
 */

#define OF_STACK_SIZE (32*1024)

#include <config.h>
#include <asm.h>

/* This code is NOT MP safe */
	.p2align 3
	. = 0x0
C_TEXT_ENTRY(ofh_start)		# make the linker happy
_ofh_cih:
	## r3 holds the parameter.  All other volatiles are available.
	mflr	r0
	# Obtain address of _ofh_work_space via a branch-and-link
	bl	_ofh_cih_continue
_ofh_work_space:
	nop
	. = 0x10
.global _ofh_tree		; 
_ofh_tree:
	.long 0xdeadbeef
	.long 0x0

	. = 0x18
.global _ofh_inited
_ofh_inited:
	.long 0x0	

	. = 0x20
.global _ofh_lastarg
_ofh_lastarg:
	.long 0x0	
	.long 0x0	
	
	. = 0x30
_ofh_cih_stack_end:
	.space	OF_STACK_SIZE
_ofh_cih_stack:
   
_ofh_cih_continue:
	mflr	r12				# r12 = &_ofh_work_space
	mr	r11, r1				# r11 = orig stk ptr

	# move to local stack
	lis	r1, (_ofh_cih_stack - _ofh_cih_stack_end) >> 16
	ori	r1, r1, (_ofh_cih_stack - _ofh_cih_stack_end) & 0xffff
	add	r1, r12, r1
	li	r10, 0					# create an initial
	STRU	r10, -(MIN_FRAME_SZ+(3*8))(r1)		#   frame with 3 slots
	STR	r11, (MIN_FRAME_SZ+(0*8))(r1)		# preserve orig stk ptr
	STR	r0, (MIN_FRAME_SZ+(1*8))(r1)		# preserve orig lr
	STR	r2, (MIN_FRAME_SZ+(2*8))(r1)		# preserve orig toc

### This should have no effect on 32 bit
	LOADADDR(r4, _ofh_work_space)
	sub	r4, r12, r4
	
	LOADADDR(r2, ofh_start)	# get the address of any function
	add	r2, r2, r4	# add the base
	LDR	r2, 8(r2)	# get the TOC for that funtion
	add	r2, r2, r4	# add the base

	bl	C_TEXT(ofh_handler)			# call handler
	LDR	r2, (MIN_FRAME_SZ+(2*8))(r1)		# restore orig toc
	LDR	r0, (MIN_FRAME_SZ+(1*8))(r1)		# restore orig lr
	LDR	r1, (MIN_FRAME_SZ+(0*8))(r1)		# restore orig stk ptr
	mtlr	r0					# return
	blr
