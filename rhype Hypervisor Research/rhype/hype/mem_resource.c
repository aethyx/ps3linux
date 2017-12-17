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
 * Memory resource control.
 *
 */

#include <config.h>
#include <hype.h>
#include <types.h>
#include <lib.h>
#include <hash.h>
#include <resource.h>
#include <lpar.h>
#include <logical.h>
#include <objalloc.h>
#include <os.h>
#include <psm.h>
#include <pmm.h>
#include <debug.h>

static uval32 mem_res_count = 0;

static sval
mem_grant(struct sys_resource **res, struct os *src, uval flags,
	  uval logical_hi, uval logical_lo, uval size, struct os *dest)
{
	(void)logical_hi;
	(void)dest;
	(void)flags;
	sval err = H_Success;
	struct logical_chunk_info *lci = laddr_to_lci(src, logical_lo);

	if (lci == NULL) {
		return H_Parameter;
	}
	if (lci->lci_ops->export == NULL) {
		return H_Resource;
	}

	struct logical_chunk_info *nlci;

	err = lci->lci_ops->export(lci, logical_lo, size, &nlci);

	if (err < 0) {
		return err;
	}

	err = resource_conflict(dest, &nlci->lci_res);
	if (err != H_Success) {
		nlci->lci_ops->destroy(nlci);
		return err;
	}


	DEBUG_OUT(DBG_MEMMGR, "%ld type: %ld export: 0x%lx[0x%lx]\n",
		  lci->lci_res.sr_cookie, flags, logical_lo, size);

	atomic_add32(&mem_res_count, 1);

	*res = &nlci->lci_res;
	return H_Success;

}

static sval
mem_accept(struct sys_resource *sr, uval *retval)
{
	struct logical_chunk_info *lci = sysres_to_lci(sr);

	if ((lci->lci_size << 1) > CHUNK_SIZE ||
	    chunk_offset_addr(lci->lci_laddr) + lci->lci_size >= CHUNK_SIZE) {
		set_lci(sr->sr_owner, 0, lci);
		*retval = lci->lci_laddr;
		lci_enable(lci);
		DEBUG_OUT(DBG_MEMMGR, "Accepting cookie %ld -> LA:0x%lx[0x%lx]\n",
			  sr->sr_cookie, lci->lci_laddr, lci->lci_size);
		return H_Success;
	}

	struct page_share_mgr *psm = sr->sr_owner->po_psm;

	if (psm == NULL) {
		psm = init_os_psm(sr->sr_owner);
	}

	sval err = psm_accept(psm, lci);

	if (err == H_Success) {
		lci_enable(lci);
		*retval = lci->lci_laddr;
		DEBUG_OUT(DBG_MEMMGR, "%s %ld LA:0x%lx[0x%lx]\n", __func__,
			  sr->sr_cookie, lci->lci_laddr, lci->lci_size);
	}
	return err;
}

static sval
mmio_accept(struct sys_resource *sr, uval *retval)
{
	struct logical_chunk_info *lci = sysres_to_lci(sr);

	set_lci(sr->sr_owner, lci->lci_raddr, lci);
	*retval = lci->lci_laddr;
	lci_enable(lci);
	DEBUG_OUT(DBG_MEMMGR, "%s %ld LA:0x%lx[0x%lx]\n", __func__,
		  sr->sr_cookie, lci->lci_laddr, lci->lci_size);

	return H_Success;
}

static sval
mem_return (struct sys_resource *res)
{
	struct logical_chunk_info *lci = sysres_to_lci(res);

	struct page_share_mgr *psm = lci->lci_os->po_psm;

	DEBUG_OUT(DBG_MEMMGR, "%s %ld LA:0x%lx[0x%lx]\n", __func__,
		  res->sr_cookie, lci->lci_laddr, lci->lci_size);

	if (psm && psm_owns(psm, lci->lci_laddr)) {
		psm_unmap_lci(psm, lci);
	} else {
		detach_lci(lci);
		lci->lci_ops->unmap(lci);

	}
	return 0;
}

static sval
mmio_return (struct sys_resource *res)
{
	struct logical_chunk_info *lci = sysres_to_lci(res);

	DEBUG_OUT(DBG_MEMMGR, "%s %ld LA:0x%lx[0x%lx]\n", __func__,
		  res->sr_cookie, lci->lci_laddr, lci->lci_size);
	detach_lci(lci);
	lci->lci_ops->unmap(lci);
	return 0;
}

static sval
mem_invalidate(struct sys_resource *res)
{				/* res is locked */
	struct logical_chunk_info *lci = sysres_to_lci(res);

	lci_disable(lci);
	return 0;
}

/* res is locked */
static sval
mem_rescind(struct sys_resource *res)
{
	struct logical_chunk_info *lci = sysres_to_lci(res);

	DEBUG_OUT(DBG_MEMMGR, "%s %ld LA:0x%lx[0x%lx]\n", __func__,
		  res->sr_cookie, lci->lci_laddr, lci->lci_size);

	atomic_add32(&mem_res_count, -1);
	lci->lci_ops->destroy(lci);
	return 0;
}

uval
define_mem(struct os *os, uval type, uval base, uval size)
{
	sval err;

	if (size == 0) {
		return INVALID_LOGICAL_ADDRESS;
	}

	struct logical_chunk_info *lci = halloc(sizeof (*lci));

	resource_init(&lci->lci_res, NULL, type);
	phys_mem_lci_init(lci, base, size);

	err = resource_conflict(os, &lci->lci_res);
	if (err < 0) {
		hfree(lci, sizeof (*lci));
		return INVALID_LOGICAL_ADDRESS;
	}


	err = insert_resource(&lci->lci_res, os);
	assert(err >= 0, "Can't give mem to os\n");
	if (err < 0) {
		hfree(lci, sizeof (*lci));
		return INVALID_LOGICAL_ADDRESS;
	}

	atomic_add32(&mem_res_count, 1);

	uval retval;

	lock_acquire(&lci->lci_res.sr_lock);
	err = accept_locked_resource(&lci->lci_res, &retval);

	assert(err == H_Success, "Can't bind resource to LA\n");

	lock_release(&lci->lci_res.sr_lock);

	hprintf("Assigned memory PA:0x%lx[0x%lx] -> LA:0x%lx\n",
		base, size, lci->lci_laddr);

	return retval;
}

static uval
mem_conflict(struct sys_resource *res1, struct sys_resource *res2)
{
	struct logical_chunk_info *lci1 = sysres_to_lci(res1);
	struct logical_chunk_info *lci2 = sysres_to_lci(res2);

	assert(lci1->lci_raddr != INVALID_PHYSICAL_ADDRESS,
	       "Expecting physical address to be set\n");
	assert(lci2->lci_raddr != INVALID_PHYSICAL_ADDRESS,
	       "Expecting physical address to be set\n");

	uval ret =ranges_conflict(lci1->lci_raddr, lci1->lci_size,
				  lci2->lci_raddr, lci2->lci_size);
	if (ret) {
		hprintf("Conflict: %lx[%lx] %lx[%lx]\n",
			lci1->lci_raddr, lci1->lci_size,
			lci2->lci_raddr, lci2->lci_size);
	}
	return ret;

}

struct resource_action mem_actions = {
	.grant_fn = mem_grant,
	.accept_fn = mem_accept,
	.return_fn = mem_return,
	.rescind_fn = mem_rescind,
	.invalidate_fn = mem_invalidate,
	.conflict_fn = mem_conflict,
};

/* MMIO is identical to MEM, except that we don't allow use of the psm
 * facilities.  It's probably important to use the native MMIO LCI's
 * lci_arch_ops
 */
struct resource_action mmio_actions = {
	.grant_fn = mem_grant,
	.accept_fn = mmio_accept,
	.return_fn = mmio_return,
	.rescind_fn = mem_rescind,
	.invalidate_fn = mem_invalidate,
	.conflict_fn = mem_conflict,
};
