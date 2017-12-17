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
#include <debug.h>
#include <h_proto.h>
#include <htab.h>
#include <hype.h>
#include <os.h>
#include <vregs.h>
#include <thread_control_area.h>
#include <vm_class.h>
#include <vm_class_inlines.h>


sval
h_sync_vm_class(struct cpu_thread *thread)
{
	struct vregs *v = thread->vregs;
	uval i = 0;

	for (; i < NUM_MAP_CLASSES; ++i) {
		struct vm_class* vmc = thread->vstate.active_cls[i];
		if (vmc == NULL) continue;

		vmc_deactivate(thread, vmc);

	}

	for (i = 0; i < NUM_MAP_CLASSES; ++i) {
		/* kernel vmc's not allowed */
		if (v->v_active_cls[i] < NUM_KERNEL_VMC) continue;

		lock_acquire(&thread->cpu->os->po_mutex);
		struct vm_class* vmc = vmc_lookup(thread, v->v_active_cls[i]);

		/* FIXME: add EA rnage checks to avoid multiple classes
		 * with the same EA range
		 */
		lock_release(&thread->cpu->os->po_mutex);

		if (vmc == NULL) continue;

		vmc_activate(thread, vmc);
		thread->vstate.active_cls[i] = vmc;
	}

	return H_Success;

}
