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
#include <asm.h>
#include <lpar.h>
#include <lib.h>
#include <htab.h>
#include <h_proto.h>
#include <hype.h>
#include <os.h>
#include <mmu.h>
#include <pmm.h>
#include <objalloc.h>
#include <bitops.h>
#include <debug.h>

static uval
htab_calc_sdr1(uval htab_addr, uval log_htab_size)
{
	uval sdr1_htabsize;
	uval htab_size = 1UL << log_htab_size;

	assert((htab_addr & (htab_size - 1)) == 0,
	       "misaligned htab address (0x%lx)\n", htab_addr);

	assert(log_htab_size <= SDR1_HTABSIZE_MAX, "htab too big (0x%lx)",
			htab_size);

	assert(log_htab_size >= HTAB_MIN_LOG_SIZE, "htab too small (0x%lx)",
			htab_size);

	sdr1_htabsize = log_htab_size - LOG_PTEG_SIZE - SDR1_HTABSIZE_BASEBITS;

	return (htab_addr | (sdr1_htabsize & SDR1_HTABSIZE_MASK));
}

/* pte_insert: called by h_enter
 *	pt: extended hpte to use
 * 	ptex: index into page-table to use
 * 	vsidWord: word containing the vsid
 * 	rpnWord: word containing the rpn
 *	lrpn: logial address corresponding to pte
 */
void
pte_insert(struct logical_htab *pt, uval ptex, const uval64 vsidWord,
	   const uval64 rpnWord, uval lrpn)
{
	union pte *chosen =
		((union pte *)(pt->sdr1 & SDR1_HTABORG_MASK)) + ptex;
	uval *shadow = pt->shadow + ptex;

	/*
	 * It's required that external locking be done to
	 * provide exclusion between the choices of insertion
	 * points.
	 * Any valid choice of pte requires that the pte be
	 * invalid upon entry to this function.
	 * XXX lock etc
	 * LPAR 18.5.4.1.2 "Algorithm" requires ldarx/stdcx etc.
	 * and using bit 57 for per-pte locking.
	 */
	assert((chosen->bits.v == 0), "Attempt to insert over valid PTE\n");

	/* Set the second word first so the valid bit is the last thing set */
	chosen->words.rpnWord = rpnWord;

	/* Set shadow word. */
	*shadow = lrpn;

	/* Guarantee the second word is visible before the valid bit */
	eieio();

	/* Now set the first word including the valid bit */
	chosen->words.vsidWord = vsidWord;
	ptesync();
}

/* htab_alloc()
 * - reserve an HTAB for this cpu
 * - set cpu's SDR1 register
 */
void
htab_alloc(struct os *os, uval log_htab_bytes)
{
	uval sdr1_val = 0;
	uval htab_raddr;
	uval htab_bytes = 1UL << log_htab_bytes;
	int i;

	htab_raddr = get_pages_aligned(&phys_pa, htab_bytes, log_htab_bytes);
	assert(htab_raddr != INVALID_PHYSICAL_PAGE, "htab allocation failure\n");

	/* hack.. should make this delta global */
	htab_raddr += get_hrmor();

	/* XXX slow. move memset out to service partition? */
	memset((void *)htab_raddr, 0, htab_bytes);

	sdr1_val = htab_calc_sdr1(htab_raddr, log_htab_bytes);

	os->htab.num_ptes = (1ULL << (log_htab_bytes - LOG_PTE_SIZE));
	os->htab.sdr1 = sdr1_val;
	os->htab.shadow = (uval *)halloc(os->htab.num_ptes * sizeof (uval));
	assert(os->htab.shadow, "Can't allocate shadow table\n");

	for (i = 0; i < os->installed_cpus; i++) {
		os->cpu[i]->reg_sdr1 = sdr1_val;
	}

	DEBUG_OUT(DBG_MEMMGR, "LPAR[0x%x] hash table: RA 0x%lx; 0x%lx entries\n",
		os->po_lpid, htab_raddr, os->htab.num_ptes);
}

void
htab_free(struct os *os)
{
	uval ht_ra = os->htab.sdr1 & SDR1_HTABORG_MASK;
	uval ht_ea;

	/* hack.. should make the hv_ra delta global */
	ht_ea = ht_ra - get_hrmor();
	if (ht_ea) {
		free_pages(&phys_pa, ht_ea,
			   os->htab.num_ptes << LOG_PTE_SIZE);
	}

	if (os->htab.shadow)
		hfree(os->htab.shadow, os->htab.num_ptes * sizeof (uval));

	DEBUG_OUT(DBG_MEMMGR, "HTAB: freed htab at 0x%lx\n", ht_ea);
}
