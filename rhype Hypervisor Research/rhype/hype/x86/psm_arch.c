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
 * Page-share manager.
 *
 */

#include <config.h>
#include <types.h>
#include <hype.h>
#include <os.h>

#include <psm.h>
#include <objalloc.h>
#include <pgalloc.h>

struct arch_mem_ops psm_arch_ops;

sval
__locked_psm_unmap_lci(struct page_share_mgr *psm,
		       struct logical_chunk_info *lci)
{
	(void)lci;
	(void)psm;
	hprintf("Need to implement removal of PTEs associated with PSMs\n");
	return 0;
}
