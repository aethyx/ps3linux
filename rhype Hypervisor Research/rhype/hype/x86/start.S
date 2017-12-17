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
 * start.S 
 *
 * The bootloader transfers control to _start. hype will be
 * linked with vma = <something>, lma = 1MB. The first thing
 * to do is to enable paging and map ourselves at that vma. 
 *
 */
#include <asm.h>
#include <config.h>
#include <hypervisor.h>
#include <mmu.h>
#include <regs.h>
#include <vm.h>
#include <stack.h> 

#define MULTIBOOT_MAGIC 	0x1badb002 
#define MULTIBOOT_FLAGS 	0x00000003
#define MULTIBOOT_CHKSUM 	(0-MULTIBOOT_MAGIC-MULTIBOOT_FLAGS)

#define MULTIBOOT_BL_MAGIC	0x2badb002

#define	LOG_LARGE_PGSIZE	LOG_PDSIZE	/* log(4MB) */
#define	NR_LARGE_PAGES		(1 << (LOG_CHUNKSIZE-LOG_LARGE_PGSIZE))

	.text 

C_TEXT_ENTRY(_start)
	jmp skip_header

	.align 4
multiboot_header:
	.long MULTIBOOT_MAGIC  /* magic number */
	.long MULTIBOOT_FLAGS  /* flags */
	.long MULTIBOOT_CHKSUM /* checksum */

skip_header:
	cli				/* no interrupts, please */

	/* make sure we were loaded by a multiboot loader */
	cmpl	$MULTIBOOT_BL_MAGIC, %eax
	jnz	load_error

	/* don't use %ebx, it contains the grub mb_info structure */

	/*
	 * Setup initial page table, this has just two mappings:
	 *
	 *	(0 to CHUNKSIZE) linear to 0 - CHUNKSIZE physical
	 *	(4G-CHUNKSIZE to 4G) linear to 0 - CHUNKSIZE physical
	 *
	 * where CHUNKSIZE should be a multiple of 4MB.
	 */
	movl	$init_pgd-HV_VBASE, %eax

	/* use 4MB page directory entries to map ourselves during boot */
	xorl	%ecx, %ecx
1:
	movl	%ecx, %edx
	shll	$22, %edx
	orl	$PTE_PS | PTE_RW | PTE_P, %edx
	movl	%edx, 0(%eax,%ecx,4)
	movl	%edx, 4*(1024-NR_LARGE_PAGES)(%eax,%ecx,4)
	incl	%ecx
	cmpl	$NR_LARGE_PAGES, %ecx
	jne	1b

	movl	%eax, %cr3		/* set page table directory */

	movl	%cr4, %eax		/* enable page size extensions */
	orl	$CR4_PSE, %eax
	movl	%eax, %cr4
	movl	%cr4, %eax		/* make sure it stuck */
	testl	$CR4_PSE, %eax
	jz	unsupported

	movl	%cr0, %eax		/* enable paging */
	orl	$CR0_PG, %eax
	movl	%eax, %cr0
	jmp	1f			/* flush TLB */
1:
	lgdt	gdtr			/* load GDT */

	movw	$__HV_DS, %ax		/* point to 3rd entry in GDT */
	movw	%ax, %ds
	movw	%ax, %ss
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs

	leal	_hype_stack, %esp
	addl 	$HV_STACK_SIZE, %esp
	movl	%esp, %ebp

	pushl	%ebx			/* save the mb info structure */

	/* after this ljmp, we're running above the link base */
	ljmp	$__HV_CS, $1f		/* point to 2nd entry in GDT */
1:
	leal	hype_init, %eax
	call 	*%eax

load_error:
	jmp	.

unsupported:
	jmp	.

	.data

/*
 * Initial page directory
 */
	.align 4096
	.global	init_pgd
init_pgd:
	.fill 4096, 1, 0

/*
 *  Global descriptor table, used just for getting started
 */
	.align 32
	.global	gdt_start
gdt_start:
	.quad 0x0000000000000000        /* reserved */

	.quad 0x00cf9a000000ffff        /* hypervisor CS */
	.quad 0x00cf92000000ffff        /* hypervisor DS */
gdt_end:

gdtr:
	.word gdt_end - gdt_start - 1	/* limit for gdt */
	.long gdt_start			/* gdt base */
