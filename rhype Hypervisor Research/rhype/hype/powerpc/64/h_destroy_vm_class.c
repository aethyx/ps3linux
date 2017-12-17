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
#include <bitops.h>
#include <h_proto.h>
#include <atomic.h>
#include <vm_class.h>
#include <vm_class_inlines.h>


sval
h_destroy_vm_class(struct cpu_thread* thread, uval id)
{
	sval ret = H_Parameter;

	struct vm_class *vmc = NULL;

	hprintf("%s %ld\n", __func__, id);
	lock_acquire(&thread->cpu->os->po_mutex);

	vmc = vmc_lookup(thread, id);

	/* should be an OR, but being a bitmap, add is ok */
	atomic_add(&vmc->vmc_flags, VMC_INVALID);

	lock_release(&thread->cpu->os->po_mutex);

	if (!vmc) return ret;

	return vmc_destroy(thread, vmc);
}
