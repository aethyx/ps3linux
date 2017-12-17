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

#include <pmm.h>
#include <os.h>

struct arch_mem_ops pmem_ops;

void
__locked_psm_cleanup(struct page_share_mgr *psm)
{
	/* Nothing do be done because we don't track PTEs, yet */
	(void)psm;
}

sval
phys_mem_unmap(struct logical_chunk_info *lci)
{
	(void)lci;
	hprintf("Need to implement PTE invalidation for phys mem LCIs\n");
	return 0;
}
