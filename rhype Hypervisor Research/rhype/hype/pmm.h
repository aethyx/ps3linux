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

#ifndef _PMM_H
#define _PMM_H

#include <config.h>
#include <hypervisor.h>
#include <os.h>
#include <partition.h>
#include <logical.h>
#include <imemcpy.h>

#define page_offset(i)	     ((i) & ((1 << LOG_PGSIZE) - 1))
#define page_number(i)	     ((i) >> LOG_PGSIZE)

struct partition_info;

extern struct arch_mem_ops pmem_ops;
extern sval phys_mem_unmap(struct logical_chunk_info *lci);
extern void __locked_psm_cleanup(struct page_share_mgr *psm);

/* initialization functions */
extern void logical_mmap_init(struct os *os);

/* management functions */
extern uval define_mem(struct os *os, uval type, uval base, uval size);
extern void logical_mmap_clean(struct os *os);
extern void logical_fill_partition_info(struct os *os);

extern uval init_logical_chunk(struct os *os, uval hint_laddr,
			       uval type, uval size);
extern uval init_extended_chunk(struct os *os, uval laddr, uval size);

#endif /* _PMM_H */
