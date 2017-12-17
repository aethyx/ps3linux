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
h_protect(struct cpu_thread *thread, uval flags, uval ptex, uval avpn)
{
	union pte *cur_htab = (union pte *)GET_HTAB(thread->cpu->os);
	union pte *pte;

	if (check_index(thread->cpu->os, ptex))
		return H_Parameter;

	pte = &cur_htab[ptex];

	/*
	 * XXX 18.5.4.1.6 does not specify behavior with respect
	 * to valid bit unset on entry.
	 * This appears to be a sane response.
	 */
	if (!pte->bits.v)
		return H_Not_Found;

	/*
	 * we are supposed to compare "bits 0-56" but these are C bits
	 * and shifted. not architected bits ??
	 */
	if ((flags & H_AVPN) && ((pte->bits.avpn << 7) != avpn))
		return H_Not_Found;

	pte->bits.v = 0;
	ptesync();
	do_tlbie(pte, ptex);

	/* We never touch pp0, and PP bits in flags are in the right
	 * order */
	pte->bits.pp1 = flags & (H_PP1 | H_PP2);
	pte->bits.n = (flags & H_N) ? 1 : 0;

	PTE_MOD_GENERAL_END(pte);

	/*
	 * 18.5.4.1.6 does not specify return values.
	 */
	return H_Success;
}
