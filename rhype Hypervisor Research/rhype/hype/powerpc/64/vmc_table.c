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
#include <types.h>
#include <cpu_thread.h>
#include <pmm.h>
#include <vregs.h>
#include <xh.h>
#include <bitops.h>
#include <objalloc.h>

static uval
vmc_table_xlate(struct vm_class *vmc, uval eaddr, union ptel *entry)
{
	uval idx = (eaddr - vmc->vmc_base_ea) >> LOG_PGSIZE;
	union linux_pte lpte;

	lpte.word = * (((uval64*)vmc->vmc_data) + idx);

	union ptel pte;
	pte.bits.rpn = lpte.bits.rpn;
	pte.bits.w = lpte.bits.w;
	pte.bits.i = lpte.bits.i;
	pte.bits.m = lpte.bits.m;
	pte.bits.g = lpte.bits.g;
	pte.bits.n = lpte.bits.n;
	pte.bits.pp1 = 1 + lpte.bits.rw;
	entry->word = pte.word;
	return lpte.bits.rpn << LOG_PGSIZE;

}


struct vm_class_ops vmc_table_ops =
{
	.vmc_xlate = vmc_table_xlate,
	.vmc_op = NULL,
};

struct vm_class*
vmc_create_table(struct cpu_thread *thr,
		 uval id, uval ea_base, uval size, uval tbl_base)
{
	uval tbl_size = (size >> LOG_PGSIZE) * sizeof(union linux_pte);

	uval tbl_ra = logical_to_physical_address(thr->cpu->os,
						  tbl_base, tbl_size);

	/* FIXME: Need to ensure that tbl_ra doesn't get invalidated on us */
	if (tbl_ra == INVALID_PHYSICAL_ADDRESS) return NULL;

	struct vm_class *vmc = halloc(sizeof(*vmc));
	vmc_init(vmc, id, ea_base, size, &vmc_table_ops);
	vmc->vmc_data = tbl_ra;

	return vmc;
}

