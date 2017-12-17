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
 * Each partition has a mailbox, into which both the
 * OS and the hv can write. This is aimed at eliminating
 * hcalls to get/set information.
 *
 * Getting the mailbox is a frequent operation, so the
 * mailbox is mapped into HV space.
 *
 */

#include <config.h>
#include <hypervisor.h>
#include <mailbox.h>
#include <mmu.h>
#include <pmm.h>
#include <lib.h>
#include <vm.h>
#include <hcall.h>
#include <h_proto.h>
#include <hype.h>
#include <pgalloc.h>

sval
h_set_mbox(struct cpu_thread *thread, uval mbox_laddr)
{
	uval mbox_paddr;
	uval mbox_vaddr = 0;

	/* make sure the logical address is within range */
	mbox_paddr = logical_to_physical_address(thread->cpu->os, mbox_laddr,
						 sizeof (struct mailbox));
	if (mbox_paddr == INVALID_PHYSICAL_ADDRESS)
		return H_Parameter;

	/* make sure the mailbox fits within one page */
	if ((mbox_laddr & (PGSIZE - 1)) + sizeof (struct mailbox) > PGSIZE)
		return H_Parameter;

	/* If there is already an mbox, reuse the HV virtual address */
	if (thread->mbox != NULL)
		mbox_vaddr = (uval)thread->mbox;

	/* map the mailbox into the hypervisor */
	mbox_vaddr =
		hv_map(mbox_vaddr, mbox_paddr & PGMASK, PGSIZE, PTE_RW, 1);
	if (mbox_vaddr == PAGE_ALLOC_ERROR)
		return H_Busy;	/* only can happen on first alloc */

	mbox_vaddr |= mbox_paddr & ~PGMASK;

#ifdef DEBUG
	hprintf("Partition mailbox for lpid 0x%x at 0x%lx\n",
		thread->cpu->os->po_lpid, mbox_paddr);
#endif

	thread->mbox = (struct mailbox *)mbox_vaddr;
	return H_Success;
}
