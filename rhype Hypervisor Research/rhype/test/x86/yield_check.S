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
 * code to verify that context switch correctly saves and
 * restores (non-volatile) registers.  First argument is LPID, 
 * second argument is array into which to store GP registers.
 *
 */

#include <asm.h>
#include <lpar.h>

	.text

/* registers checked: ebx, esp, ebp, cs, ds, es, fs, esi, edi */
C_TEXT_ENTRY(hcall_yield_check_regs)
	push	%ebp
	movl	%esp, %ebp
	movl	0xc(%ebp), %edx	/* get array pointer */

	movl 	%ebx, (%edx)
	movl	%esp, 4(%edx)
	movl	%ebp, 8(%edx)
	xor	%eax, %eax
	movw	%cs, %ax
	movl	%eax, 12(%edx)
	movw	%ds, %ax
	movl	%eax, 16(%edx)
	movw	%es, %ax
	movl	%eax, 20(%edx)
	movw	%fs, %ax
	movl	%eax, 24(%edx)
	movw	%gs, %ax
	movl	%eax, 28(%edx)
	movl	%esi, 32(%edx)
	movl	%edi, 36(%edx)

	/* do hcall_yield(NULL, H_SELF_LPID) */
	push	$H_SELF_LPID
	push	$0
	call	hcall_yield
	add	$8, %esp	/* eax contains the return value */
	
	/* now compare the current values to the saved reg values */
	movl	0xc(%ebp), %edx	/* get array pointer, this also means ebp is ok */

#define CHECK(reg, n)		\
	cmpl	%reg, n(%edx); 	\
	jnz	check_failed;

#define CHECK_SEGR(reg, n)		\
	movw	%reg, %cx;		\
	cmpl	%ecx, n(%edx);		\
	jnz	check_failed;

	CHECK(ebx, 0)
	CHECK(esp, 4)
	CHECK(ebp, 8)

	xor 	%ecx, %ecx
	CHECK_SEGR(cs, 12)
	CHECK_SEGR(ds, 16)
	CHECK_SEGR(es, 20)
	CHECK_SEGR(fs, 24)
	CHECK_SEGR(gs, 28)
	
	CHECK(esi, 32)
	CHECK(edi, 36)

	/* we got thro ok */
	jmp	1f
check_failed:
	movl	$H_Hardware, %eax

1:
	pop	%ebp
	ret
