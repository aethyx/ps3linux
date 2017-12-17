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
#include <htab.h>
#include <vm.h>

sval
h_read(struct cpu_thread *thread, uval flags, uval ptex)
{
	union pte *cur_htab = (union pte *)GET_HTAB(thread->cpu->os);
	int pte_count;

#ifdef DEBUG_READ_PTX
	hprintf("rptx: 0x%lx\n", ptex);
#endif
	if (CHECK_UNSUP(flags))
		return H_Parameter;

	if (check_index(thread->cpu->os, ptex))
		return H_Parameter;

	if (flags & H_READ_4) {
		/* check bounds on all 4 PTEs */
		if (check_index(thread->cpu->os, ptex + 1) ||
		    check_index(thread->cpu->os, ptex + 2) ||
		    check_index(thread->cpu->os, ptex + 3))
			return H_Parameter;
	}

	if (!(flags & H_READ_4)) {
		/* return this PTE in regs 4 and 5 */
		save_pte(&cur_htab[ptex], thread->cpu->os->htab.shadow + ptex,
			 thread, 4, 5);
	} else {
		ptex &= ~0x3;	/* dump half a PTEG */

		for (pte_count = 0; pte_count < 4; pte_count++) {
			save_pte(&cur_htab[ptex + pte_count],
				 thread->cpu->os->htab.shadow + ptex +
				 pte_count, thread, 4 + pte_count * 2,
				 5 + pte_count * 2);
		}
	}

	return H_Success;
}
