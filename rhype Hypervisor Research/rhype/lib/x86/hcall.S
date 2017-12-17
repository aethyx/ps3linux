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
 * move the arguments from the stack into registers and then do an int 0x81
 *
 */


#include <asm.h>
#include <lpar.h>

/*
 *	The stack looks like this:
 *	Stack  |  Accessed as	  | 	Loaded in reg
 *	arg 6		28(%ebp)	edi
 *	arg 5		24(%ebp)	esi
 *	arg 4		20(%ebp)	edx
 *	arg 3		16(%ebp)	edx
 *	arg 2		12(%ebp)	ecx 
 *	retvals		 8(%ebp)	 -
 *	ret
 *	old_ebp	<- ebp
 *	ebx	(call saved)
 *	esi
 *	edi
 */

/*
 * The first 5 arguments are transfered in registers,
 * any remaining arguments (up to 16) stay on the stack.
 */
	.macro LOAD_INPUTS n_inputs
	.if \n_inputs > 4
	movl	28(%ebp), %edi
	.endif
	.if \n_inputs > 3
	movl	24(%ebp), %esi
	.endif
	.if \n_inputs > 2
	movl	20(%ebp), %ebx
	.endif
	.if \n_inputs > 1
	movl	16(%ebp), %edx
	.endif
	.if \n_inputs > 0
	movl	12(%ebp), %ecx
	.endif
	.endm

/*
 * Currently we save all clobbered registers onto the stack, this is
 * clearly subobtimal.
 */
#define SAVE_CLOBBER_LIST	\
	pushl	%ebx;		\
	pushl	%esi;		\
	pushl	%edi;

#define RESTORE_CLOBBER_LIST	\
	popl %edi;		\
	popl %esi;		\
	popl %ebx;

/*
 * The output values after a hcall are in the registers.
 * eax contains the return code, and should be left as is.
 * Other outputs are in sequence: ecx, edx, ebx, esi, edi; these must be
 * moved into the retvals array.
 */

	/* %eax contains the pointer to retvals array */
	.macro STORE_OUTPUTS n_args
	.if \n_args > 0
	/* need to get &retvals */
	pushl	%eax			
	movl	8(%ebp), %eax		
	movl %ecx, 0x0(%eax)
	.endif
	.if \n_args > 4
	movl %edi, 0x10(%eax)
	.endif
	.if \n_args > 3
	movl %esi, 0xc(%eax)
	.endif
	.if \n_args > 2
	movl %ebx, 0x8(%eax)
	.endif
	.if \n_args > 1		
	movl %edx, 0x4(%eax)
	.endif
	.if \n_args > 5
	movl	$\n_args-5, %ecx
	movl	%eax, %edi
	addl	$0x14, %edi 
	movl	%esp, %esi
	addl	$4, %esi
	cld
	rep movsl
	.endif
	.if \n_args > 0
	popl	%eax
	.endif
	.if \n_args > 5
	addl	$(\n_args-5)*4, %esp
	.endif
	.endm /* STORE_OUTPUS */


	.macro RESERVE_OUTPUTS_SPACE n_args
	.if	\n_args > 5
	subl	$(\n_args-5)*4, %esp
	.endif
	.endm /* RESERVE_OUTPUTS_SPACE */

	
/* call as hcall_func_name(u32 retvals[], arg1, ..., argN) */
#define HCALL(n_inputs, n_outputs, func_name, opcode)	\
	C_TEXT_ENTRY(func_name)				\
	pushl 	%ebp;					\
	movl	%esp, %ebp;				\
	movl	$opcode, %eax;				\
	SAVE_CLOBBER_LIST;				\
	LOAD_INPUTS	n_inputs;			\
	RESERVE_OUTPUTS_SPACE	n_outputs; 		\
	int	$0xF0;					\
	STORE_OUTPUTS	n_outputs;			\
	RESTORE_CLOBBER_LIST;				\
	pop	%ebp;					\
	ret

	
/* Get the common interfaces */
#include <lib/hcall_common.S>

/* Note: The # of inputs does _not_ include both the opcode, and 
 * the retvals argument */
HCALL(2, 0, hcall_page_dir, H_PAGE_DIR)
HCALL(2, 0, hcall_flush_tlb, H_FLUSH_TLB)
HCALL(8, 0, hcall_read, H_READ)
HCALL(6, 0, hcall_put_term_char, H_PUT_TERM_CHAR)
HCALL(1, 5, hcall_get_term_char, H_GET_TERM_CHAR)
HCALL(8, 0, hcall_hypervisor_data, H_HYPERVISOR_DATA)
HCALL(4, 0, hcall_dt_entry, H_DT_ENTRY)
HCALL(0, 1, hcall_get_pfault_addr, H_GET_PFAULT_ADDR)
HCALL(1, 0, hcall_set_mbox, H_SET_MBOX)
HCALL(2, 0, hcall_sys_stack, H_SYS_STACK)
HCALL(9, 8, hcall_dr, H_DR)
