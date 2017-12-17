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

#include <asm.h>
#include <hypervisor.h>

#define TEST_STACK_SIZE 0x40000

.text

C_TEXT_ENTRY(_text_start) /* for V2P() */
C_TEXT_ENTRY(_start)
	jmp __start

	/* at offset 0x10 is a pointer to the partition info */
	. = 0x10
_partition_info: 
	.quad HYPE_PARTITION_INFO_MAGIC_NUMBER 
        .long pinfo 

__start:
	/* initialize stack */
	movl	$teststack, %esp
	movl	$teststack, %ebp

	call	crt_init
	jmp	.

.data

	## Used as stack for this test os
teststackguard:
	.long 0x01234567
	.space TEST_STACK_SIZE - MIN_FRAME_SZ
teststack:
	. = . + MIN_FRAME_SZ

