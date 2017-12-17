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

#include <config.h>
#include <h_proto.h>
#include <hype.h>
#include <hcall.h>
#include <partition.h>
#include <pmm.h>

static sval
get_pinfo(struct os *os, uval laddr)
{
	uval paddr;

	paddr = logical_to_physical_address(os, laddr,
					    sizeof (struct partition_info));

	if (paddr == INVALID_PHYSICAL_ADDRESS) {
		return H_Parameter;
	}

	copy_out((void *)paddr, &os->cached_partition_info[1],
		 sizeof (os->cached_partition_info[1]));
	return 0;
}

static sval
get_meminfo(struct cpu_thread *thread, uval type, uval start_laddr)
{
	(void)type;
	uval laddr = ALIGN_DOWN(start_laddr, CHUNK_SIZE);
	struct logical_chunk_info *lci = laddr_to_lci(thread->cpu->os,
						      start_laddr);

	if (lci && lci->lci_res.sr_type != type) {
		lci = NULL;
	}

	if (lci && !lci_owns(lci, start_laddr)) {
		if (start_laddr >= lci->lci_laddr + lci->lci_size) {
			laddr = ALIGN_UP(start_laddr, CHUNK_SIZE);
			lci = NULL;
		} else {
			laddr = ALIGN_DOWN(start_laddr, CHUNK_SIZE);
		}
	}

	while ((lci == NULL) && (laddr < MAX_LOGICAL_ADDRESS)) {

		lci = laddr_to_lci(thread->cpu->os, laddr);
		if (lci && lci->lci_res.sr_type != type) {
			lci = NULL;
		}

		laddr += CHUNK_SIZE;
	}

	if (!lci) {
		return_arg(thread, 1, INVALID_LOGICAL_ADDRESS);
		return_arg(thread, 2, INVALID_LOGICAL_ADDRESS);
	} else if (lci->lci_raddr == INVALID_PHYSICAL_ADDRESS) {
		/* Non-simple translation, could have holes.  So we simply mark
		 * that this chunk is mapped, but we don't have more details,
		 * by returning a size of 0.
		 */
		return_arg(thread, 1, lci->lci_laddr);
		return_arg(thread, 2, 0);
	} else {
		return_arg(thread, 1, lci->lci_laddr);
		return_arg(thread, 2, lci->lci_size);
	}
	return H_Success;
}

sval
h_lpar_info(struct cpu_thread *thread, uval cmd, uval arg1,
	    uval arg2, uval arg3, uval arg4)
{
	switch (cmd) {
	case H_GET_PINFO:
		return get_pinfo(thread->cpu->os, arg1);
	case H_GET_MEM_INFO:
		return get_meminfo(thread, MEM_ADDR, arg1);
	}
	(void)thread;
	(void)cmd;
	(void)arg1;
	(void)arg2;
	(void)arg3;
	(void)arg4;

	return 0;
}
