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

#ifndef _VM_H
#define _VM_H

#include <config.h>
#include <types.h>
#include <mmu.h>
#include <pmm.h>

/* CHECK_* return codes:
 * 0 - passed check
 * non-zero - failed check
 */

/* done modifying PTE, general case */
#define PTE_MOD_GENERAL_END(ptePtr) \
	eieio(); \
	ptePtr->bits.v = 1; \
	ptesync();

#define UNSUP_FLAGS	(	H_ICACHE_INVALIDATE			\
				| H_ICACHE_SYNCHRONIZE			\
				| H_ZERO_PAGE				\
				| H_R_XLATE				\
			)

#define CHECK_UNSUP(flags) (flags & UNSUP_FLAGS)

#define VM_MAP_SUPPORTED_FLAGS (     \
  H_VM_MAP_ICACHE_INVALIDATE      |  \
  H_VM_MAP_ICACHE_SYNCRONIZE      |  \
  H_VM_MAP_INVALIDATE_TRANSLATION |  \
  H_VM_MAP_INSERT_TRANSLATION     |  \
  H_VM_MAP_LARGE_PAGE             |  \
  H_VM_MAP_ZERO_PAGE              |  \
  0x00000000000000F               )

static inline int
check_index(struct os *os, uval ptex)
{
	return (ptex > os->htab.num_ptes);
}

extern void do_tlbie(union pte *pte, uval ptex);

/* save_pte():
 * store the 2 words of a PTE to specified regs in struct cpu_thread struct,
 * translating the RPN logical->physical.
 *
 * PTE_X entry includes logical page number info as provided by h_enter
 *
 * XXX for debugging, don't translate to real when rpn=0 (the client OS sees 2
 * zero words).
 */
static inline void
save_pte(union pte *ptePtr, uval *exdata,
	 struct cpu_thread *thread, int vsidIndex, int rpnIndex)
{
	union pte localPte;	/* so we can use localPte.bits.rpn */

	localPte.words.rpnWord = ptePtr->words.rpnWord;

	thread->reg_gprs[vsidIndex] = ptePtr->words.vsidWord;

	if (localPte.words.rpnWord != 0) {
		localPte.bits.rpn = (*exdata);
	}

	thread->reg_gprs[rpnIndex] = localPte.words.rpnWord;
}

#endif /* ! _VM_H */
