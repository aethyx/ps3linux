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
h_create_vm_class(struct cpu_thread* thread,
		  uval type, uval id, uval ea_base, uval size,
		  uval imp_arg1, uval imp_arg2, uval imp_arg3, uval imp_arg4)
{

	(void)imp_arg2;
	(void)imp_arg3;
	(void)imp_arg4;
	sval ret = H_Parameter;

	struct vm_class *vmc = NULL;

	lock_acquire(&thread->cpu->os->po_mutex);

	vmc = vmc_lookup(thread, id);
	if (vmc) {
		ret = H_UNAVAIL;
		goto done;
	}

	switch (type) {
	case H_VM_CLASS_LINEAR:
		vmc = vmc_create_linear(id, ea_base, size, imp_arg1);
		break;
	case H_VM_CLASS_REFLECT:
		vmc = vmc_create_reflect(id, ea_base, size);
		break;
	case H_VM_CLASS_TABLE:
		vmc = vmc_create_table(thread, id, ea_base, size, imp_arg1);
		break;
	}

	if (!vmc) goto done;

	hprintf("%s %ld ea: 0x%lx\n", __func__, id, ea_base);
	if (id < NUM_KERNEL_VMC) {
		if (!cas_ptr(&thread->cpu->os->vmc_kernel[id], NULL, vmc)) {
			assert(!thread->cpu->os->vmc_kernel[id],
			       "Kernel VMC id already defined\n");
		}
	} else {
		ht_insert(&thread->cpu->os->vmc_hash, &vmc->vmc_hash);
	}

	ret = H_Success;
done:
	lock_release(&thread->cpu->os->po_mutex);
	return ret;
}
