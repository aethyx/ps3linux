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
 * Low level exception handling code.
 *
 */

#include <asm.h>
#include <asm_defs.h>
#include <lpar.h>
#include <idt.h>
#include <context.h>
#include <vm.h>

/*
 * The code in this file deals with interrupt handling. The general idea
 * is to invoke a C handler as quickly as possible and let it figure out
 * how to virtualize the resource.
 *
 * On x86 we use the following dispatching policy:
 *
 *	trapno < 0x20
 *		These are Intel assigned exceptions/faults which are
 *		dispatched to exception(). Most of these faults are
 *		reflected to the current partition.
 *	trapno >= 0x20 && trapno < 0x30
 *		This is where the external (device) interrupts are
 *		mapped which are handled by interrupt.
 *	trapno >= 0x30 && trapno < 0xF0
 *		These are user defined traps. These are always reflected
 *		to the current partition.
 *	trapno >= 0xF0
 *		These are hypervisor reserved traps. Only code in the
 *		partition running in ring 1 and 2 are allowed to issue
 *		hypervisor traps. Ring 3 code (typically the application)
 *		traps are reflected to the partition OS.
 *
 * FIXME! This design needs to be revisited by the time we have accurate
 * profiling data. There are obvious short cuts but they do not make
 * sense until we have have a better understanding about the costs.
 */

/*
 * All processor traps/interrupts/faults end up here.
 *
 * On an exception/fault, the processor pushes CS:EIP, SS, ESP and an
 * optional error code onto the stack. The common_exception routine
 * below saves the processor context and transfers control to exception()
 * whose job it is to virtualize and pass on the trap.
 */
	.macro	EXCEPTION_HANDLER trapno error
	.text
	.align	16
1:	.if	\error == 0
	pushl	$0			/* dummy error code */
	.endif
	pushl	$\trapno
	jmp	common_exception
	.data
	.long	1b
	.text
	.endm

	.data
	.align	4
	.global	exception_handlers
exception_handlers:
	EXCEPTION_HANDLER	0, 0	/* divide error */
	EXCEPTION_HANDLER	1, 0	/* debug */
	EXCEPTION_HANDLER	2, 0	/* NMI interrupt */
	EXCEPTION_HANDLER	3, 0	/* breakpoint */
	EXCEPTION_HANDLER	4, 0	/* overflow */
	EXCEPTION_HANDLER	5, 0	/* BOUND range exceeded */
	EXCEPTION_HANDLER	6, 0	/* invalid opcode */
	EXCEPTION_HANDLER	7, 0	/* device not available */
	EXCEPTION_HANDLER	8, 1	/* double fault */
	EXCEPTION_HANDLER	9, 0	/* coprocessor segment overrun */
	EXCEPTION_HANDLER	10, 1	/* invalid TSS */
	EXCEPTION_HANDLER	11, 1	/* segment not present */
	EXCEPTION_HANDLER	12, 1	/* stack-segment fault */
	EXCEPTION_HANDLER	13, 1	/* general protection */
	EXCEPTION_HANDLER	14, 1	/* page fault */
	EXCEPTION_HANDLER	15, 0	/* reserved */
	EXCEPTION_HANDLER	16, 0	/* FPU floating-point error */
	EXCEPTION_HANDLER	17, 1	/* alignment check */
	EXCEPTION_HANDLER	18, 0	/* machine check */
	EXCEPTION_HANDLER	19, 0	/* SIMD floating-point error */
	EXCEPTION_HANDLER	20, 0	/* reserved */
	EXCEPTION_HANDLER	21, 0	/* reserved */
	EXCEPTION_HANDLER	22, 0	/* reserved */
	EXCEPTION_HANDLER	23, 0	/* reserved */
	EXCEPTION_HANDLER	24, 0	/* reserved */
	EXCEPTION_HANDLER	25, 0	/* reserved */
	EXCEPTION_HANDLER	26, 0	/* reserved */
	EXCEPTION_HANDLER	27, 0	/* reserved */
	EXCEPTION_HANDLER	28, 0	/* reserved */
	EXCEPTION_HANDLER	29, 0	/* reserved */
	EXCEPTION_HANDLER	30, 0	/* reserved */
	EXCEPTION_HANDLER	31, 0	/* reserved */

	.text
	.align	16
common_exception:			/* common exception handler */
	cld
	PUSH_SEGR
	PUSH_GPRS
	RESTORE_HV_SEGMENTS

#ifdef USE_GDB_STUB     
	testl	$0x3, 12*4(%esp)	/* exception occurred in HV if */
	jz	hv_debug		/*     the fault-time CS has RPL 0 */
#endif /* USE_GDB_STUB */
	
	/* use %eax for PCO address */
	call	get_thread_addr
	orl	%eax, %eax
	/* if PCO (needed by C handlers) is NULL, spin forever */
	jz	.

	POP_GPRS_TO_MEM(%eax, /* PCO */)
	POP_SEGR_TO_MEM(%eax, /* PCO */)
	popl	%ebx			/* trap number */
	popl	%ecx			/* error code */
	POP_EXCEPTION_STATE_TO_MEM(%eax, /* PCO */)
	movl	%gs, GS(%eax)
	movl	%fs, FS(%eax)
	pushl	%eax			/* thread */
	pushl	%ecx
	pushl	%ebx
	pushl	%eax
	call	exception		/* exception(thread, trapno, errcode) */
	addl	$12, %esp		/* gcc may have clobbered these! */
	call	resume_OS		/* resume_OS(thread) */
	jmp	.
	/* NOT REACHED */

#ifdef USE_GDB_STUB
hv_debug:
	LOADADDR(%eax, gdb_currdbg)	/* state */
	POP_GPRS_TO_MEM(%eax, GDB_)
	POP_SEGR_TO_MEM(%eax, GDB_)
	popl	%ebx			/* trap number */
	popl	%ecx			/* error code */
	POP_EXCEPTION_STATE_TO_MEM(%eax, GDB_)
	movl	%gs, GDB_GS(%eax)
	movl	%fs, GDB_FS(%eax)
	pushl	%ebx
	pushl	%eax
	call	enter_gdb		/* state/eax=enter_gdb(state,trapno) */
	addl	$8, %esp		/* may have been clobbered */

    	/* restore non-essential stack regs */
	movl	GDB_ES(%eax), %es
	movl	GDB_FS(%eax), %fs
	movl	GDB_GS(%eax), %gs

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

	/* save values to stack for iret */
	PUSH_EXCEPTION_STATE_FROM_MEM(%eax, GDB_)

	/*
	 * DS makes the thread pointer inaccessible, so the final EAX value
	 * must be on the stack.
	 */
	pushl	GDB_EAX(%eax)
	movl	GDB_DS(%eax), %ds
	popl	%eax
	iret			/* return to regularly scheduled programming */
#endif /* USE_GDB_STUB */
	
/*
 * All external interrupts end up here.
 *
 * On an exception/fault, the processor pushes CS:EIP, SS, ESP onto the
 * stack. The common_interrupt below saves the processor context and
 * transfers control to interrupt() whose job it is to virtualize and
 * pass on the interrupt.
 */
	.macro	INTERRUPT_HANDLER irq
	.text
	.align	16
1:	pushl	$\irq
	jmp	common_interrupt
	.data
	.long	1b
	.text
	.endm

	.data
	.align	4
	.global	interrupt_handlers
interrupt_handlers:
	vector=BASE_IRQ_VECTOR
	.rept	NR_INTERRUPT_HANDLERS
	INTERRUPT_HANDLER	vector
	vector=vector+1
	.endr

	.text
	.align	16
common_interrupt:			/* common interrupt handler */
	cld
	PUSH_SEGR
	PUSH_GPRS
	RESTORE_HV_SEGMENTS

	/* use %eax for PCO address */
	call	get_thread_addr
	orl	%eax, %eax
	/* if PCO (needed by C handlers) is NULL, spin forever */
	jz	.

	POP_GPRS_TO_MEM(%eax, /* PCO */)
	POP_SEGR_TO_MEM(%eax, /* PCO */)
	popl	%ebx			/* interrupt number */
	POP_EXCEPTION_STATE_TO_MEM(%eax, /* PCO */)
	movl	%gs, GS(%eax)
	movl	%fs, FS(%eax)
	pushl	%eax			/* thread */
	pushl	%ebx
	pushl	%eax
	call	interrupt		/* interrupt(thread, vector) */
	addl	$8, %esp		/* gcc may have clobbered these! */
	call	resume_OS		/* resume_OS(thread) */
	jmp	.
	/* NOT REACHED */


/*
 * All traps end up here.
 *
 * On a trap, the processor pushes CS:EIP, SS, ESP onto the stack.
 * The common_trap routine below saves the processor context and
 * transfers control to trap() whose job it is to virtualize and
 * pass on the trap. Except for the hypervisor calls, all traps
 * are passed on to the active partition.
 */
	.macro	TRAP_HANDLER trapno
	.text
	.align	16
1:	pushl	$\trapno
	jmp	common_trap
	.data
	.long	1b
	.text
	.endm

	.data
	.align	4
	.global	trap_handlers
trap_handlers:
	vector=BASE_TRAP_VECTOR
	.rept	NR_TRAP_HANDLERS
	TRAP_HANDLER	vector
	vector=vector+1
	.endr

	.text
	.align	16
common_trap:				/* common trap handler */
	cld
	PUSH_SEGR
	PUSH_GPRS
	RESTORE_HV_SEGMENTS

	/* use %eax for PCO address */
	call	get_thread_addr
	orl	%eax, %eax
	/* if PCO (needed by C handlers) is NULL, spin forever */
	jz	.

	POP_GPRS_TO_MEM(%eax, /* PCO */)
	POP_SEGR_TO_MEM(%eax, /* PCO */)
	popl	%ebx			/* trap number */
	POP_EXCEPTION_STATE_TO_MEM(%eax, /* PCO */)
	movl	%gs, GS(%eax)
	movl	%fs, FS(%eax)
	pushl	%eax			/* thread */
	pushl	%ebx
	pushl	%eax
	call	trap			/* trap(thread, trapno) */
	addl	$8, %esp		/* gcc may have clobbered these! */
	call	resume_OS		/* resume_OS(thread) */
	jmp	.
	/* NOT REACHED */

