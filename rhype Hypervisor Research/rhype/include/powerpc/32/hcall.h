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

#ifndef _PPC32_HCALL_H
#define _PPC32_HCALL_H

#include <include/hcall.h>
struct cpu_thread;

extern sval h_remove(struct cpu_thread *pcop);
extern sval h_enter(struct cpu_thread *pcop, uval flags, uval tlbx,
		    uval epnWord, uval rpnWord, uval attribWord);
extern sval h_read(struct cpu_thread *pcop);
extern sval h_clear_mod(struct cpu_thread *pcop);
extern sval h_clear_ref(struct cpu_thread *pcop);
extern sval h_protect(struct cpu_thread *pcop);
extern sval h_get_tce(struct cpu_thread *pcop);
extern sval h_put_tce(struct cpu_thread *pcop);

#endif /* _PPC32_HCALL_H */
