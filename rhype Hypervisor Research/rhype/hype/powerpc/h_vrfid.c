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
#include <vdec.h>
#include <thread_control_area.h>


sval
h_vrfid(struct cpu_thread *thread, uval restore_idx, uval new_dec)
{
	struct vexc_save_regs *vr =&thread->vregs->vexc_save[restore_idx];
	struct thread_control_area *tca = get_tca();

	if (restore_idx >= NUM_EXC_SAVES) return H_Parameter;

	if (new_dec) {
		partition_set_dec(thread, new_dec);
	}

	uval val = vr->v_srr1;
	val |= V_LPAR_MSR_ON;
	val &= ~V_LPAR_MSR_OFF;
	tca->srr1 = val;
	set_v_msr(thread, vr->v_srr1);
	tca->srr0 = vr->v_srr0;

	thread->vregs->active_vsave = vr->prev_vsave;
	tca->save_area = vr;
	return vr->reg_gprs[3];
}
