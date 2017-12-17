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
 * head.S
 */
#include <asm.h>
#include <lpar.h>
#include <hypervisor.h>

#define STACK_SIZE 	16*1024

#define LOAD_INPUTS_0
#define LOAD_INPUTS_1		\
	movl	12(%ebp), %ecx;	\
	LOAD_INPUTS_0
#define LOAD_INPUTS_2		\
	movl	16(%ebp), %edx;	\
	LOAD_INPUTS_1
#define LOAD_INPUTS_3		\
	movl	20(%ebp), %ebx;	\
	LOAD_INPUTS_2
#define LOAD_INPUTS_4		\
	movl	24(%ebp), %esi;	\
	LOAD_INPUTS_3
#define LOAD_INPUTS_5		\
	movl	28(%ebp), %edi;	\
	LOAD_INPUTS_4

#define SAVE_CLOBBER_LIST	\
	pushl	%ebx;		\
	pushl	%esi;		\
	pushl	%edi;

#define RESTORE_CLOBBER_LIST	\
	popl %edi;		\
	popl %esi;		\
	popl %ebx;

#define HCALL(n_inputs, n_outputs, func_name, opcode)	\
	C_TEXT_ENTRY(func_name)		\
	pushl 	%ebp;			\
	movl	%esp, %ebp;		\
	movl	$opcode, %eax;		\
	SAVE_CLOBBER_LIST;		\
	LOAD_INPUTS_##n_inputs;		\
	int	$0xF0;			\
	/* need to get &retvals */	\
	pushl	%eax;			\
	movl	8(%ebp), %eax;		\
	call	store_outputs_ ## n_outputs;	\
	popl	%eax;			\
	RESTORE_CLOBBER_LIST;		\
	pop	%ebp;			\
	ret


/*
 * Bootstrap loader start.
 *
 * The hypervisor starts here with:
 *
 *	%eax = 0x48563836 = "HV86"
 *	%ecx = 0 = version
 *	%esi = pointer to command line
 *
 * We pass %esi on to lboot which in turn passes it on to the Linux kernel.
 */
	.section .head, "ax"
	.global	_start
_start:
	jmp 1f

	/* at offset 0x10 is a pointer to the partition info */
	. = 0x10
_partition_info: 
	.quad HYPE_PARTITION_INFO_MAGIC_NUMBER 
        .long partinfo 

	.align	16
1:
	/* initialize stack */
	movl	$stack, %esp
	movl	$stack, %ebp

	pushl	%esi			/* command line argument */
	call	lboot
	jmp	.


/*
 * Start Linux kernel.
 *
 * Linux expects the following:
 *	%ebx - CPU number
 *	%esi - boot param
 */
	.text
	.align	16
	.global	start_kernel
start_kernel:
	movl	4(%esp), %eax		/* load address */
	movl	8(%esp), %ebx		/* CPU number */
	movl	12(%esp), %esi		/* boot params */
	jmp	*%eax


/*
 * The output values after a hcall are in the registers.
 * eax contains the return code, and should be left as is.
 * Other outputs are in sequence: ecx, edx, ebx, esi, edi; these must be
 * moved into the retvals array.
 */

	.text
	.align	16

/* %eax contains the pointer to retvals array */
store_outputs_5:	movl %edi, 0x10(%eax)
store_outputs_4:	movl %esi, 0xc(%eax)
store_outputs_3:	movl %ebx, 0x8(%eax)
store_outputs_2:	movl %edx, 0x4(%eax)
store_outputs_1:	movl %ecx, 0x0(%eax)
store_outputs_0:	ret

HCALL(1, 0, hcall_yield, H_YIELD)
HCALL(5, 0, hcall_put_term_char, H_PUT_TERM_CHAR)
HCALL(1, 4, hcall_get_term_char, H_GET_TERM_CHAR)
HCALL(5, 5, hcall_lpar_info, H_LPAR_INFO)

	.bss
	.align	4

	.space STACK_SIZE - MIN_FRAME_SZ
stack:
	. = . + MIN_FRAME_SZ

