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

#ifndef _X86_TEST_H
#define _X86_TEST_H

#include <test/test.h>
#include <mailbox.h>

extern struct mailbox mbox;

static inline void
rrupts_on(void)
{
	mbox.IF = 1;		/* sti */
}

static inline void
rrupts_off(void)
{
	mbox.IF = 0;		/* cli */
}

extern void crt_init(void);
extern void halt(void);
extern void setup_idt(void);
extern void set_trap_gate(uval, uval16, uval32, uval16);
extern void set_intr_gate(uval, uval, uval);

extern void do_pfault(uval ax, uval cx, uval dx, uval bx,
		      uval ebp, uval esi, uval edi,
		      uval ret, uval cr2, uval err, uval eip,
		      uval cs, uval eflags);
extern uval prom_load(struct partition_status* ps, uval ofd);

#endif /* _X86_TEST_H */
