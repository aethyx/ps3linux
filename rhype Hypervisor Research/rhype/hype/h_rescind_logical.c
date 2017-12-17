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
#include <hype.h>
#include <types.h>
#include <lib.h>
#include <hcall.h>
#include <hash.h>
#include <resource.h>
#include <lpar.h>
#include <objalloc.h>
#include <os.h>
#include <h_proto.h>
#include <pmm.h>

sval
h_rescind_logical(struct cpu_thread *thread, uval flags, uval cookie)
{

	(void)thread;
	struct dlist todo;
	struct sys_resource *sr;
	sval err = get_sys_resource(thread->cpu->os, cookie, &sr);

	if (err != H_Success) {
		return err;
	}

	/* If not a forced rescind, then we need to check that there are
	 * no children cookies and the resource is not in use.
	 * If these conditions are satisfied, we can run the "forced" path,
	 * knowing how it will execute.
	 */
	if (!(flags & 1) &&
	    (sr->sr_flags & RES_MAPPED || !dlist_empty(&sr->sr_children))) {
		return H_Resource;
	}

	dlist_init(&todo);
	err = forced_rescind_mark(sr, &todo, NULL);

	/* Has the resource been fully rescinded?
	 * Is the rescind in progress?
	 * How do we know it's fully complete?
	 * What should we return here?
	 */
	if (err == H_Rescinded) {
		return H_Success;
	}

	/* Wait for a generation count update */
	/* This is necessary to allow any current hcalls that use the
	 * resources we've just marked to finish.  We also have to be
	 * sure that none of the hcalls we want to finish are using
	 * rcu_pause().  This is the only rescind sequence that can
	 * safely use rcu_pause(), since being at the hcall level we
	 * can make assertions about what locks are held and what
	 * pointers are in use.
	 */
	rcu_pause();

	err = forced_rescind_cleanup(&todo);
	return err;
}
