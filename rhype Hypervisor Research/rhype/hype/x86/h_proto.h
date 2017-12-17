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
 * Declarations for x86 specific hypervisor hcall()-implementation 
 * functions.
 *
 */
#ifndef __HYPE_X86_H_PROTO_H__
#define __HYPE_X86_H_PROTO_H__

#include <config.h>
#include <types.h>
#include <hypervisor.h>
#include <os.h>
#include <cpu_thread.h>

#include_next <h_proto.h>

extern sval h_gdt_entry(struct cpu_thread *thread,
			uval index, uval word0, uval word1);
extern sval h_idt_entry(struct cpu_thread *thread,
			uval index, uval word0, uval word1);
extern sval h_dt_entry(struct cpu_thread *thread, uval type,
		       uval index, uval word0, uval word1);

extern sval h_get_pte(struct cpu_thread *thread, uval flags, uval vaddr);
extern sval h_get_pfault_addr(struct cpu_thread *thread);
extern sval h_sys_stack(struct cpu_thread *thread, uval ss, uval esp);
extern sval h_page_dir(struct cpu_thread *thread, uval flags,
		       uval pgdir_laddr);
extern sval h_set_mbox(struct cpu_thread *thread, uval mbox_laddr);
extern sval h_flush_tlb(struct cpu_thread *thread, uval flags, uval vaddr);
extern sval h_dr(struct cpu_thread *thread, uval selector, uval dr0,
		 uval dr1, uval dr2, uval dr3, uval dr4, uval dr5,
		 uval dr6, uval dr7);

#endif /* __HYPE_X86_H_PROTO_H__ */
