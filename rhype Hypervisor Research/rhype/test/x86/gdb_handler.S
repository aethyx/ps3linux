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
 * debug/breakpoint interrupt handler: save state and call enter_gdb()
 */

#include <asm.h>
#include <asm_defs.h>
#include <idt.h>
	
#define PUSHA	\
	pushl	%ss;	\
	pushl	%ds;	\
	pushl	%es;	\
	pushl	%eax;	\
	pushl	%ecx;	\
	pushl	%edx;	\
	pushl	%ebx;	\
	pushl	%esi;	\
	pushl	%edi;	\
	pushl	%ebp

#define POPA_TO_MEM(cpu_state)		\
	popl	GDB_EBP(cpu_state);	\
	popl	GDB_EDI(cpu_state);	\
	popl	GDB_ESI(cpu_state);	\
	popl	GDB_EBX(cpu_state);	\
	popl	GDB_EDX(cpu_state);	\
	popl	GDB_ECX(cpu_state);	\
	popl	GDB_EAX(cpu_state);	\
	popl	GDB_ES(cpu_state);	\
	popl	GDB_DS(cpu_state);	\
	popl	GDB_SS(cpu_state)

#define POPA_EXCEPTION_STATE_TO_MEM(cpu_state)	\
	popl	GDB_EIP(cpu_state);	\
	popl	GDB_CS(cpu_state);	\
	popl	GDB_EFLAGS(cpu_state)

	.text
		
C_TEXT_ENTRY(gdb_handler)
	cld
	PUSHA
	LOADADDR(%eax, gdb_currdbg)
	POPA_TO_MEM(%eax)
	POPA_EXCEPTION_STATE_TO_MEM(%eax)
	movl	%esp, GDB_ESP(%eax)
	movl	%gs, GDB_GS(%eax)
	movl	%fs, GDB_FS(%eax)

	pushl	$0			/* exception_type */
	pushl	%eax			/* *state */
	call	enter_gdb
	addl	$8, %esp		/* may have been clobbered */

    	/* restore non-essential stack regs */
	movl	GDB_ES(%eax), %es
	movl	GDB_FS(%eax), %fs
	movl	GDB_GS(%eax), %gs
	movl	GDB_SS(%eax), %ss

	/*
	 * Restore GPRs except for EAX, which holds our thread pointer. The
	 * GPRs hold our return arguments, which is why we even restore the
	 * volatiles.
	 */
	movl	GDB_EBX(%eax), %ebx
	movl	GDB_ECX(%eax), %ecx
	movl	GDB_EDX(%eax), %edx
	movl	GDB_ESI(%eax), %esi
	movl	GDB_EDI(%eax), %edi
	movl	GDB_EBP(%eax), %ebp
	movl	GDB_ESP(%eax), %esp

	/* save values to stack for iret */
	pushl	GDB_EFLAGS(%eax)
	pushl	GDB_CS(%eax)
	pushl	GDB_EIP(%eax)

	/*
	 * DS makes the thread pointer inaccessible, so the final EAX value
	 * must be on the stack.
	 */
	pushl	GDB_EAX(%eax)
	movl	GDB_DS(%eax), %ds
	popl	%eax
	HCALL_IRET
	jmp     .
