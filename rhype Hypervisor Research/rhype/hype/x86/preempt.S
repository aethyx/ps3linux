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
 * Low level code to transfer control to the partition.
 */
#include <asm.h>
#include <asm_defs.h>
#include <mmu.h>
#include <vm.h>
#include <context.h>
#include <regs.h>


/*
 * void resume_OS(struct cpu_thread *thread)
 */
	.text
	.align	16
C_TEXT_ENTRY(resume_OS)
	movl	0x4(%esp), %ebx 	/* ebx = thread pointer */

	pushl	%ebx			/* get the thread pointer to restore */
	call	update_ticks
	popl	%ebx

	/* preempt the current thread, if necessary */
	movb	PREEMPT_THREAD(%ebx), %al
	orb	%al, %al
	jz	1f
	call	preempt
	movl	%eax, %ebx
1:

	/* perform outstanding RCUs */
	pushl	%ebx
	call	rcu_check
	popl	%ebx

#ifdef HV_EXIT_DEBUG
	pushl	%ebx
	call	arch_validate
	popl	%ebx
#endif /* HV_EXIT_DEBUG */

	/* switch to the new idt */
	lidt	IDTR(%ebx)

	/* clear the "busy" bit in the TSS descriptor in the new gdt */
	movl	(GDTR+2)(%ebx), %eax
	andb	$0xfd, (__TSS+5)(%eax)

	/* switch to the new gdt before loading any segment registers */
	lgdt	GDTR(%ebx)

	/* reload the task register because the gdt has changed */
	movw	$__TSS, %ax
	ltr	%ax

	/* restore non-essential segment regs */
	movl	ES(%ebx), %es
	movl	FS(%ebx), %fs
	movl	GS(%ebx), %gs

	/* don't write to CR3 if it hasn't changed, avoiding a TLB flush */
	movl	%cr3, %eax
	cmp	%eax, CR3(%ebx)
	je	1f
	movl	CR3(%ebx), %eax
	movl	%eax, %cr3
1:

	/* restore CR0, CR2, and CR4. CR1 is reserved */
	movl	CR0(%ebx), %eax
	movl	%eax, %cr0
	movl	CR2(%ebx), %eax
	movl	%eax, %cr2
	movl	CR4(%ebx), %eax
	movl	%eax, %cr4

	/*
	 * Restore GPRs except for EBX, which holds our thread pointer. The
	 * GPRs hold our return arguments, which is why we even restore the
	 * volatiles.
	 */
	movl	EAX(%ebx), %eax
	movl	ECX(%ebx), %ecx
	movl	EDX(%ebx), %edx
	movl	ESI(%ebx), %esi
	movl	EDI(%ebx), %edi
	movl	EBP(%ebx), %ebp

	/* save values to stack for iret */
	PUSH_EXCEPTION_STATE_FROM_MEM(%ebx, /* PCO */)

	/*
	 * DS makes the thread pointer inaccessible, so the final EBX value
	 * must be on the stack.
	 */
	pushl	EBX(%ebx)
	movl	DS(%ebx), %ds
	pop	%ebx
	iret            /* return to kernel code */


/*
 * Preempt the currently running OS and run the OS chosen by the scheduler 
 * currently this is called only from the resume_OS path, and assumes that
 * the thread pointer is in ebx
 */
	.text
	.align	16
preempt:
	testl	$CPUID_FEATURES_FXSAVE, cpuid_features
	jz	preempt_without_sse

preempt_with_sse:
	/* save _all_ state */
	SAVE_FP_MMX_SSE  %ebx
	SAVE_DR	  ebx
	SAVE_TR	  ebx
	SAVE_LDTR ebx

	pushl	%ebx			/* get the thread pointer to restore */
	call	preempt_thread
	popl	%ebx
	orl	%eax, %eax		/* check for NULL */
	jz	idle

	RESTORE_LDTR eax
	RESTORE_TR   eax
	RESTORE_DR   %eax, %ecx
	RESTORE_FP_MMX_SSE  %eax
	ret

preempt_without_sse:
	/* save _all_ state */
	SAVE_FP_MMX  %ebx
	SAVE_DR	  ebx
	SAVE_TR	  ebx
	SAVE_LDTR ebx

	pushl	%ebx			/* get the thread pointer to restore */
	call	preempt_thread
	popl	%ebx
	orl	%eax, %eax		/* check for NULL */
	jz	idle

	RESTORE_LDTR eax
	RESTORE_TR   eax
	RESTORE_DR   %eax, %ecx
	RESTORE_FP_MMX  %eax
	ret

/*
 * Idle loop
 */
	.text
	.align	16
idle:
	sti				/* necessary? */
	hlt
	jmp	idle

