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
 *
 * Interrupt definitions.
 *
 */
#ifndef __X86_IDT_H__
#define __X86_IDT_H__

/* sizes of various tables */
#define	NR_EXCEPTIONS_HANDLERS	32
#define	NR_INTERRUPT_HANDLERS	32
#define	NR_TRAP_HANDLERS	192
#define	NR_IDT_ENTRIES		256

/* trap bases */
#define	BASE_IRQ_VECTOR		0x20
#define	BASE_TRAP_VECTOR	0x40
#define BASE_HCALL_VECTOR	0xF0

/* size of each entry in bytes */
#define	SIZEOF_IDTENTRY		8

/* flags in an IDT gate descriptor */
#define GATE_MASK		0x9fff	/* specify everything except dpl */
#define TASK_GATE		0x8500	/* present, 32-bit task gate */
#define INTR_GATE		0x8E00	/* present, 32-bit interrupt gate */
#define TRAP_GATE		0x8F00	/* present, 32-bit trap gate */

/* Protected Mode Exceptions/Interrupt vectors */
#define DE_VECTOR		0
#define DB_VECTOR		1
#define NI_VECTOR		2
#define BP_VECTOR		3
#define OF_VECTOR		4
#define BR_VECTOR		5
#define UD_VECTOR		6
#define NM_VECTOR		7
#define DF_VECTOR		8
#define SO_VECTOR		9
#define TS_VECTOR		10
#define NP_VECTOR		11
#define SS_VECTOR		12
#define GP_VECTOR		13
#define PF_VECTOR		14
#define MF_VECTOR		16
#define AC_VECTOR		17
#define MC_VECTOR		18
#define XF_VECTOR		19

/* Hypervisor software interrupt vectors */
#define HCALL_VECTOR		0xF0
#define HCALL_VECTOR_IRET	0xF1
#define HCALL_IRET		int	$HCALL_VECTOR_IRET

/* IRQ defines */
#define TIMER_IRQ		0
#define TIMER_VECTOR		(BASE_IRQ_VECTOR + TIMER_IRQ)

#define XIR_IRQ			31
#define XIR_VECTOR		(BASE_IRQ_VECTOR + XIR_IRQ)

#endif /* __X86_IDT_H__ */
