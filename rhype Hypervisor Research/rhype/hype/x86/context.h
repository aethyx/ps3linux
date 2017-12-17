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
 * Processor context save/restore macros
 *
 */
#ifndef __HYPE_X86_CONTEXT_H__
#define __HYPE_X86_CONTEXT_H__

/*
 * The registers to be saved/restored are as follows:
 * 1. On entry into hv, we need to save all the state that the hv uses:
 *	GPRs, [ EFLAGS, CS, SS, ] DS, ES
 * The [ .. ] are the registers automatically pushed/restored by the processor
 * 2. On exit from the hv, back to the same OS, we need to restore
 * everything that was clobbered by the hv. This is basically the above
 * set of registers + CR0
 *	GPRs, [ EFLAGS, CS, SS, ] DS, ES, CR0
 * 3. The full state of an OS needs to be saved if preempting the OS.
 * This includes in addition to 1 above,
 *	FP, MMX, SSE, (CR, DR, TR, LDTR, GDTR?)
 * The (..) registers are already in the pco? FIXME
 * 4. Scheduling in a new OS, or running the first OS involves:
 *	Everything in 2 above
 *	FP, MMX, SSE, CR, DR, TR, LDTR, GDTR?
 * The TLBs are flushed automatically. Cached values (I/DCache) are not a problem?
 */

#define PUSH_SEGR	\
	pushl	%ds;	\
	pushl	%es;

#define PUSH_GPRS	\
	pushl	%eax;	\
	pushl	%ecx;	\
	pushl	%edx;	\
	pushl	%ebx;	\
	pushl	%esi;	\
	pushl	%edi;	\
	pushl	%ebp;

#define POP_SEGR_TO_MEM(thread, prefix)		\
	popl	prefix##ES(thread);	\
	popl	prefix##DS(thread);

#define POP_GPRS_TO_MEM(thread, prefix)	\
	popl	prefix##EBP(thread);	\
	popl	prefix##EDI(thread);	\
	popl	prefix##ESI(thread);	\
	popl	prefix##EBX(thread);	\
	popl	prefix##EDX(thread);	\
	popl	prefix##ECX(thread);	\
	popl	prefix##EAX(thread);

#define POP_EXCEPTION_STATE_TO_MEM(thread, prefix)	\
	popl	prefix##EIP(thread);		\
	popl	prefix##CS(thread);		\
	popl	prefix##EFLAGS(thread);		\
	testl	$0x3, prefix##CS(thread);	\
	jz	1f;				\
	popl	prefix##ESP(thread);		\
	popl	prefix##SS(thread);		\
	jmp	2f;				\
1:	movl	%esp, prefix##ESP(thread);	\
	movl	%ss, prefix##SS(thread);	\
2:

#define PUSH_EXCEPTION_STATE_FROM_MEM(thread, prefix)	\
	testl	$0x3, prefix##CS(thread);	\
	jz	1f;				\
	pushl	prefix##SS(thread);		\
	pushl	prefix##ESP(thread);		\
1:	pushl	prefix##EFLAGS(thread);		\
	pushl	prefix##CS(thread);		\
	pushl	prefix##EIP(thread);		\

/* any entry into the hv which accesses data, or calls
 * a C function, needs to set the CS, DS, ES and SS to hv values
 * CS, SS are set by the processor, so we just restore DS and ES */
/* *INDENT-OFF* */
.macro RESTORE_HV_SEGMENTS
	movl	$__HV_DS, %eax
	movw	%ax, %ds
	movw 	%ax, %es
.endm

.macro RESTORE_FP_MMX thread
	frstor	FP_MMX_SSE(\thread)
.endm

.macro	SAVE_FP_MMX thread
	fnsave	FP_MMX_SSE(\thread)
	fwait
.endm

.macro RESTORE_FP_MMX_SSE thread
	fxrstor	FP_MMX_SSE(\thread)
.endm

.macro	SAVE_FP_MMX_SSE thread
	fxsave	FP_MMX_SSE(\thread)
	fwait
.endm

#define set_dr(thread, reg, tmp)	\
	movl	DR ## reg(thread), tmp;	\
	movl	tmp, %db ## reg
.macro RESTORE_DR thread, tmp
	movl	DR7(\thread), \tmp
	orl	\tmp, \tmp
	jz	1f
	set_dr(\thread, 0, \tmp)
	set_dr(\thread, 1, \tmp)
	set_dr(\thread, 2, \tmp)
	set_dr(\thread, 3, \tmp)
	set_dr(\thread, 6, \tmp)
	set_dr(\thread, 7, \tmp)
	jmp	2f
1:
	movl	\tmp, %db7
2:
.endm
#undef set_dr

.macro	SAVE_DR	thread
.endm

.macro RESTORE_TR thread
.endm

.macro	SAVE_TR	thread
.endm

.macro RESTORE_LDTR thread
.endm

.macro	SAVE_LDTR thread
.endm
/* *INDENT-OFF* */
#endif /* __HYPE_X86_CONTEXT_H__ */
