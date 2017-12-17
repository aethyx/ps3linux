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
#include <lib.h>
#include <mmu.h>
#include <pmm.h>
#include <os.h>
#include <partition.h>
#include <hcall.h>
#include <logical.h>
#include <util.h>
#include <atomic.h>
#include <objalloc.h>
#include <h_proto.h>
#include <ipc.h>

static sval
phys_mem_export(struct logical_chunk_info *lci, uval laddr, uval size,
		struct logical_chunk_info **plci)
{
	if (!range_subset(lci->lci_laddr, lci->lci_size, laddr, size)) {
		return H_Parameter;
	}

	struct logical_chunk_info *new_lci;
	new_lci = halloc(sizeof (struct logical_chunk_info));
	uval offset = laddr - lci->lci_laddr;

	resource_init(&new_lci->lci_res, &lci->lci_res, lci->lci_res.sr_type);
	phys_mem_lci_init(new_lci, lci->lci_raddr + offset, size);

	*plci = new_lci;
	return size;
}

static void
__phys_mem_destroy(struct rcu_node *rcu)
{
	struct logical_chunk_info *lci;
	lci = RCU_PARENT(struct logical_chunk_info, lci_rcu, rcu);

	hfree(lci, sizeof (*lci));
}

static sval
phys_mem_destroy(struct logical_chunk_info *lci)
{
	rcu_defer(&lci->lci_rcu, __phys_mem_destroy);
	return 0;
}

extern sval phys_mem_unmap(struct logical_chunk_info *lci);

static struct chunk_ops phys_mem_ops = {
	.unmap = phys_mem_unmap,
	.export = phys_mem_export,
	.destroy = phys_mem_destroy,
};

/* Type is logical addr type as per RPA 21.2.1.5.1 */
sval
h_mem_define(struct cpu_thread *thread, uval type, uval addr, uval size)
{
	uval caller = thread->cpu->os->po_lpid;

	assert(type == MEM_ADDR || type == MMIO_ADDR,
	       "Unknown memory type being defined: %ld\n", type);
	if (caller != ctrl_os->po_lpid) {
		hprintf("LPAR[%lu] is not controller\n", caller);
		return H_Parameter;
	}

	uval ret = define_mem(thread->cpu->os, type, addr, size);

	if (ret == INVALID_LOGICAL_ADDRESS) {
		return H_Parameter;
	}

	return_arg(thread, 1, ret);
	return H_Success;
}

void
phys_mem_lci_init(struct logical_chunk_info *lci, uval raddr, uval size)
{
	lci->lci_ops = &phys_mem_ops;
	lci->lci_arch_ops = &pmem_ops;
	lci->lci_size = size;
	lci->lci_laddr = chunk_offset_addr(raddr);
	lci->lci_raddr = raddr;
}
