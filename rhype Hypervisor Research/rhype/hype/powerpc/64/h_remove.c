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
#include <logical.h>

/*
 * 18.5.4.1.1 (H_REMOVE) specifies: "use the proper tlbie
 * instruction for the page size within a critical section
 * protected by the proper lock, and 18.5.4.1.2 (H_ENTER)
 * in the "Algorithm" subsection specifies the acquisition
 * of the lock bit, presumably to synchronize against H_REMOVE
 * and friends.
 */

sval
h_remove(struct cpu_thread *thread, uval flags, uval ptex, uval avpn)
{
	union pte *cur_htab = (union pte *)GET_HTAB(thread->cpu->os);
	union pte *pte;
	uval *shadow;

	if (check_index(thread->cpu->os, ptex))
		return H_Parameter;

	/*
	 * XXX acquire & release per-pte lock (bit 57)
	 * specified in 18.5.4.1.1
	 */

	pte = &cur_htab[ptex];
	shadow = &thread->cpu->os->htab.shadow[ptex];

	if ((flags & H_AVPN) && ((pte->bits.avpn << 7) != avpn))
		return H_Not_Found;

	if ((flags & H_ANDCOND) && ((avpn & pte->words.vsidWord) != 0))
		return H_Not_Found;

	/* return old PTE in regs 4 and 5 */
	save_pte(pte, shadow, thread, 4, 5);

	/* XXX - I'm very skeptical of doing ANYTHING if not bits.v */
	/* XXX - I think the spec should be questioned in this case (MFM) */
	if (pte->bits.v) {
		struct logical_chunk_info *lci;
		uval laddr = *shadow << LOG_PGSIZE;

		lci = laddr_to_lci(thread->cpu->os, laddr);

		if (!lci)
			return H_Parameter;

		if (lci->lci_arch_ops->amo_rem_ptep) {
			lci->lci_arch_ops->amo_rem_ptep(lci, laddr,
							pte->bits.rpn, pte);
		}
		atomic_add32(&lci->lci_ptecount, ~0);	/* subtract 1 */
	}

	/*
	 * invalidating the pte is the only update required,
	 * though the memory consistency requirements are large.
	 */
	pte->bits.v = 0;
	ptesync();
	do_tlbie(pte, ptex);

	return H_Success;
}
