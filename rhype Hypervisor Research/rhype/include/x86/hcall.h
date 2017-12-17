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

#ifndef _X86_HCALL_H_
#define _X86_HCALL_H_

#include <config.h>
#include <types.h>
#include_next <hcall.h>

/* constants for the type field of the dt_entry hcall */
#define	H_DT_GDT		0	/* global descriptor table */
#define	H_DT_IDT		1	/* interrupt descriptor table */

/* hcall_page_dir flags */
#define	H_PGD_FLUSH 		0x01	/* flush page cache */
#define	H_PGD_PREFETCH		0x02	/* prefetch shadow pages */
#define	H_PGD_ACTIVATE		0x04	/* set mapping */

/* h_dr selector flags */
#define H_DR0 (1 << 0)
#define H_DR1 (1 << 1)
#define H_DR2 (1 << 2)
#define H_DR3 (1 << 3)
#define H_DR4 (1 << 4)
#define H_DR5 (1 << 5)
#define H_DR6 (1 << 6)
#define H_DR7 (1 << 7)

extern uval hcall_dt_entry(uval *retvals, uval type,
			   uval index, uval word0, uval word1);
extern uval hcall_page_dir(uval *retvals, uval flags, uval pgdir_laddr);
extern uval hcall_flush_tlb(uval *retvals, uval flags, uval vaddr);
extern uval hcall_get_pfault_addr(uval *retvals);
extern uval hcall_set_mbox(uval *retvals, uval mbox_laddr);
extern uval hcall_dr(uval *retvals, uval selector, uval dr0, uval dr1,
		     uval dr2, uval dr3, uval dr4, uval dr5, uval dr6,
		     uval dr7);

#endif /* _X86_HCALL_H_ */
