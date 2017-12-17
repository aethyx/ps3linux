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
#include <hba.h>
#include <lib.h>
#include <hcall.h>
#include <eic.h>
#include <hype.h>
#include <hv_regs.h>

lock_t eic_lock = 0;

struct cpu_thread *eic_owner = NULL;
extern sval eic_config(uval arg1, uval arg2, uval arg3, uval arg4);

static uval
eic_owner_dead(struct os *os, struct lpar_event *le, uval event)
{
	(void)os;
	(void)le;
	(void)event;
	lock_acquire(&eic_lock);
	eic_owner = NULL;
	lock_release(&eic_lock);
	return H_Success;
}

struct lpar_event eic_owner_death = {
	.le_mask = LPAR_DIE,
	.le_func = eic_owner_dead,
};

static sval
eic_set_owner(uval lpid, uval cpu, uval thr)
{
	struct os *os = os_lookup(lpid);
	struct cpu_thread *thread;

	if (!os) {
		return H_Parameter;
	}

	thread = get_os_thread(os, cpu, thr);

	if (thread != NULL) {
		if (eic_owner) {
			unregister_event(eic_owner->cpu->os, &eic_owner_death);
		}

		eic_owner = thread;
		register_event(os, &eic_owner_death);

		return H_Success;
	}
	return H_Parameter;
}

extern sval eic_set_line(uval lpid, uval cpu, uval thr,
			 uval hw_src, uval *isrc);

struct cpu_thread *
eic_default_thread(void)
{
	struct cpu_thread *thread;

	assert(eic_owner != NULL, "no EIC owner\n");
	thread = get_os_thread(eic_owner->cpu->os, get_proc_id(),
			       get_thread_id());

	if (!thread)
		thread = eic_owner;

	return thread;
}

sval
h_eic_config(struct cpu_thread *thread, uval command,
	     uval arg1, uval arg2, uval arg3, uval arg4)
{
	sval rc = H_Parameter;

	if (thread->cpu->os != ctrl_os &&
	    thread->cpu->os != eic_owner->cpu->os) {
		return H_Privilege;
	}

	lock_acquire(&eic_lock);

	switch (command) {
	case CONFIG:
		rc = eic_config(arg1, arg2, arg3, arg4);
		break;
	case MAP_LINE:{
			uval isrc;

			rc = eic_set_line(arg1, arg2, arg3, arg4, &isrc);
			if (rc == H_Success) {
				return_arg(thread, 1, isrc);
			}
			break;
		}
	case CTL_LPAR:
		rc = eic_set_owner(arg1, arg2, arg3);
		break;
	}
	lock_release(&eic_lock);
	return rc;
}
