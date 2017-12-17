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
 * Allocate and deallocate OS-related data structures.
 * This is globally locked -- if you are creating and destroying
 * OSes quickly enough to stress this lock, you have many other
 * more serious problems...
 *
 */

#include <config.h>
#include <os.h>
#include <slb.h>

/* switch_slb()
 *
 * The spec says that: "[an] slbmte must be preceded by an slbie (or
 * slbia) instruction that invalidates the existing translation."
 *
 * and then says: "No slbie (or slbia) is needed if the slbmte
 * instruction replaces a valid SLB entry with a mapping of a
 * different ESID (e.g., to satisfy an SLB miss).
 *
 * It may be possible that the same ESID on two partions exist for
 * different translations so better safe then sorry.
 */
void
switch_slb(struct cpu_thread *curThread, struct cpu_thread *nextThread)
{
	struct slb_entry *e = curThread->slb_entries;
	int i;

	/* save all extra SLBs */
	for (i = 1; i < SWSLB_SR_NUM; i++) {
		uval vsid;
		uval esid;

		/* *INDENT-OFF* */
		__asm__ __volatile__(
			"slbmfev	%0,%2	\n\t"
			"slbmfee	%1,%2	\n\t"
			: "=&r" (vsid), "=&r" (esid)
			: "r" (i)
			: "memory");
		/* *INDENT-ON* */

		/* FIXME: should we bother to save invalid entries? */
		e[i].slb_vsid = vsid;
		e[i].slb_esid = esid;
#ifdef SLB_DEBUG
		if (vsid != 0) {
			hprintf("%s: LPAR[0x%x]: S%02d: 0x%016lx 0x%016lx\n",
				__func__, curThread->cpu->os->po_lpid,
				i, vsid, esid);
		}
#endif
	}

	/* invalidate all including SLB[0], we manually have to
	 * invalidate SLB[0] since slbia gets the rest */
	/* FIXME: review all of these isyncs */

	slbia();
	if (e[0].slb_esid & (1 << (63 - 36))) {
		uval64 rb;
		uval64 class;

		class = (e->slb_vsid >> (63 - 56)) & 1ULL;
		rb = e[0].slb_esid & (~0ULL << (63 - 35));
		rb |= class << (63 - 36);

		slbie(rb);
	}

	e = nextThread->slb_entries;

	/* restore all extra SLBs */
	for (i = 1; i < SWSLB_SR_NUM; i++) {
		uval vsid = e[i].slb_vsid;
		uval esid = e[i].slb_esid;

		/* FIXME: should we bother to restore invalid entries */
		/* stuff in the index here */
		esid |= i & ((0x1UL << (63 - 52 + 1)) - 1);

		/* *INDENT-OFF* */
		__asm__ __volatile__(
			/* FIXME: do we have to isync for every one? */
			"isync			\n\t"
			"slbmte		%0,%1	\n\t"
			"isync			\n\t"
			:
			: "r" (vsid), "r" (esid)
			: "memory");
		/* *INDENT-ON* */

#ifdef SLB_DEBUG
		if (vsid != 0) {
			hprintf("%s: LPAR[0x%x]: R%02d: 0x%016lx 0x%016lx\n",
				__func__, nextThread->cpu->os->po_lpid,
				i, vsid, esid);
		}
#endif
	}
}

void
save_slb(struct cpu_thread *thread)
{
	struct slb_entry *slb_entry = thread->slb_entries;
	int i;

	/* save all extra SLBs */
	for (i = 0; i < SWSLB_SR_NUM; i++) {
		uval vsid;
		uval esid;

		/* *INDENT-OFF* */
		__asm__ __volatile__("slbmfev	%0,%2	\n\t"
				     "slbmfee	%1,%2	\n\t":"=&r"(vsid),
				     "=&r"(esid)
				     :"r"    (i)
				     :"memory");
		/* *INDENT-ON* */

		/* FIXME: should we bother to save invalid entries? */
		slb_entry[i].slb_vsid = vsid;
		slb_entry[i].slb_esid = esid;
#ifdef SLB_DEBUG
		if (vsid != 0) {
			hprintf("%s: LPAR[0x%x]: S%02d: 0x%016lx 0x%016lx\n",
				__func__, thread->cpu->os->po_lpid,
				i, vsid, esid);
		}
#endif
	}
}

void
restore_slb(struct cpu_thread *thread)
{
	struct slb_entry *slb_entry = thread->slb_entries;
	int i;

	/* restore all extra SLBs */
	for (i = 0; i < SWSLB_SR_NUM; i++) {
		uval vsid = slb_entry[i].slb_vsid;
		uval esid = slb_entry[i].slb_esid;

		/* FIXME: should we bother to restore invalid entries */
		/* stuff in the index here */
		esid |= i & ((0x1UL << (63 - 52 + 1)) - 1);

		/* *INDENT-OFF* */
		__asm__ __volatile__(
			/* FIXME: do we have to isync for every one? */
			"isync			\n\t"
			"slbmte		%0,%1	\n\t"
			"isync			\n\t"
			:
			:"r" (vsid), "r"(esid)
			:"memory");
		/* *INDENT-ON* */

#ifdef SLB_DEBUG
		if (vsid != 0) {
			hprintf("%s: LPAR[0x%x]: R%02d: 0x%016lx 0x%016lx\n",
				__func__, thread->cpu->os->po_lpid,
				i, vsid, esid);
		}
#endif
	}
}
