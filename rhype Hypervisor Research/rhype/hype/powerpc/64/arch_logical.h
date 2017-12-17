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
#ifndef _ARCH_LOGICAL_H
#define _ARCH_LOGICAL_H

/* Meant to be included from logical.h only */

#include <htab.h>

struct logical_chunk_info;
struct cpu_thread;

/* This may be used as a sort of generic amo_unmap function */
extern sval hpte_scan_unmap(struct logical_chunk_info *lci);

struct arch_mem_ops {
	uval (*amo_sav_ptep) (struct logical_chunk_info * lci,
			      uval laddr, uval rpn, union pte * pte);
	uval (*amo_rem_ptep) (struct logical_chunk_info * lci,
			      uval laddr, uval rpn, union pte * pte);
	uval (*amo_check_WIMG) (struct logical_chunk_info * lci,
				uval laddr, union ptel pte);
};

#endif /* _ARCH_LOGICAL_H */
