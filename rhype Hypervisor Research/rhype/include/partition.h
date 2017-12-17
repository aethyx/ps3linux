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

#ifndef _PARTITION_H
#define _PARTITION_H

#include <hypervisor.h>

#define MAX_MEM_RANGES 5
#define INVALID_MEM_RANGE (~((uval)0))

#define INVALID_VIRTUAL_ADDRESS  -1UL

struct mem_range {
	uval addr;
	uval size;
};

#define LARGE_PAGE_SIZE_INVALID (0xFFFFFFFFFFFFFFFFULL)
#define LARGE_PAGE_SIZE64K (64 * 1024)
#define LARGE_PAGE_SIZE1M (1024 * 1024)
#define LARGE_PAGE_SIZE16M (16 * LARGE_PAGE_SIZE1M)

/* The struct partition_info struct is partially filled in by the OS
 * at OS image creation time, used by the hypervisor to set some
 * things up, and filled in by the hypervisor to inform the OS of
 * decisions made by the hypervisor.
 */

/* An offset to this data structure in the RMO area should be placed
 * at offset 0x18, if the OS supports it.  A magic number of
 * HYP_PARTITION_INFO_MAGIC_NUMBER defined in hypervisor.h must be
 * placed at offset 0x10 of the RMO area.
 */

struct partition_info {
	uval large_page_size1;
	uval large_page_size2;
	uval mbox;		/* x86 mailbox (FIXME! remove) */
	struct mem_range mem[MAX_MEM_RANGES];
	uval32 lpid;
	uval32 sfw_tlb;
};

#endif /* _PARTITION_H */
