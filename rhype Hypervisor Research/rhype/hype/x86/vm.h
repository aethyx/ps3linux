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
/*
 * Virtual memory definitions.
 *
 */
#ifndef __HYPE_X86_VM_H__
#define __HYPE_X86_VM_H__

/*
 * The hypervisor address space is from HV_BASE to 4GB.
 * LTEXT is set in x86/Makefile.isa.
 */
#define	HV_VBASE	(LTEXT - 0x00100000)
#define	HV_VSIZE	(~0UL - HV_VBASE + 1)

/* the base at which the hypervisor and OS are loaded */
#define	HV_LINK_BASE	0x100000
#define	OS_LINK_BASE	0x100000

#ifndef __ASSEMBLY__
#include <imemcpy.h>

/* hypervisor page directory */
extern uval *hv_pgd;

/* operations to manage hypervisor virtual memory space */
extern uval hv_map(uval, uval, uval, uval, uval);
extern void hv_unmap(uval vaddr, uval size);
extern uval virtual(uval);
extern uval physical(uval);
extern void invalidate_tlb(uval, uval);

/* operations to handle partition virtual memory space */
extern uval v2p(struct cpu_thread *, uval);
extern int ispresent(struct cpu_thread *, uval, uval, uval, uval *, uval *);
extern int isstackpresent(struct cpu_thread *, uval, uval, uval *, uval *);

/*
 * When moving data back and forth between partitions using
 * guest virtual addresses (typically stack manipulations)
 * we can optimize this considerably if the MMU context is
 * already mapped.
 * The switch(size) is there to help the optimizer to make
 * the correct tradeoffs for the fast path.
 */
static inline void *
get_lpar(struct cpu_thread *thread, void *hvdst, uval vaddr, uval size)
{
	if (likely(thread->pgd->pgdir.hv_paddr == get_cr3())) {
		/* easy, it is already mapped */
		switch (size) {
		case sizeof (uval32):
			*(uval32 *)hvdst = *(uval32 *)vaddr;
			break;
		case sizeof (uval64):
			*(uval64 *)hvdst = *(uval64 *)vaddr;
			break;
		default:
			memcpy(hvdst, (void *)vaddr, size);
			break;
		}
	} else {
		/* use the imemcpy mechanism */
		uval paddr = v2p(thread, vaddr);

		if (paddr == (uval)-1) {
			return NULL;
		}
		if (copy_in(hvdst, (void *)paddr, size) == NULL) {
			return NULL;
		}
	}
	return hvdst;
}

static inline void *
put_lpar(struct cpu_thread *thread, void *hvsrc, uval vaddr, uval size)
{
	if (likely(thread->pgd->pgdir.hv_paddr == get_cr3())) {
		/* easy, it is already mapped */
		switch (size) {
		case sizeof (uval32):
			*(uval32 *)vaddr = *(uval32 *)hvsrc;
			break;
		case sizeof (uval64):
			*(uval64 *)vaddr = *(uval64 *)hvsrc;
			break;
		default:
			memcpy((void *)vaddr, hvsrc, size);
			break;
		}
	} else {
		/* use the imemcpy mechanism */
		uval paddr = v2p(thread, vaddr);

		if (paddr == (uval)-1)
			return NULL;
		if (copy_out((void *)paddr, hvsrc, size) == NULL)
			return NULL;
	}
	return hvsrc;
}

#endif /* __ASSEMBLY__ */

#endif /* __HYPE_X86_VM_H__ */
