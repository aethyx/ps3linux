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
#include <types.h>
#include <mmu.h>
#include <hcall.h>
#include <lpar.h>
#include <lib.h>
#include <htab.h>

extern uval log_htab_bytes;

uval
get_esid(uval ea)
{
	return (ea & (~0ULL << (63 - 35)));
}

uval
get_vsid(uval ea)
{
	uval vsid_val;
	uval esid_val = get_esid(ea);

	/* bring down the ESID */
	esid_val >>= 63 - 35;

	/* FIXME: WHAT the hell is this hack! */
	if (esid_val == 0xc00000000ULL) {
		vsid_val = 0x06a99b4b14;
	} else {
		vsid_val = esid_val;
	}
	return vsid_val;
}

static uval
get_pteg(uval vaddr, uval vsid, uval pg_size)
{
	uval hash;
	uval vpn = vaddr >> (pg_size);
	uval mask = (1 << (28 - pg_size)) - 1;
	uval htab_bytes = 1UL << log_htab_bytes;
	uval hash_mask = (htab_bytes >> LOG_PTEG_SIZE) - 1;

	hash = (vsid & VSID_HASH_MASK) ^ (vpn & mask);

	return hash & hash_mask;
}

int
pte_find_free(struct partition_info *pi, uval eaddr, uval8 l, uval8 ls)
{
	int i;
	uval rc;
	uval ret[8]; /* depending on flags we could get 8 values */
	uval flags = 0;
	uval idx;
	union pte *ptep = (union pte *)ret; /* only interested in first two */
	uval lp = get_log_pgsize(pi, l, ls);

	idx = NUM_PTES_IN_PTEG * get_pteg(eaddr, get_vsid(eaddr), lp);

	for (i = 0; i < NUM_PTES_IN_PTEG; i++) {
		rc = hcall_read(ret, flags, idx + i);

		assert((rc == H_Success),
		       "H_READ: FAILURE: returned %ld\n", rc);

		if (ptep->bits.v == 0) {
			return (idx + i);
		}
	}
	return -1;
}

int
pte_find_entry(struct partition_info *pi, uval eaddr, uval8 l, uval8 ls)
{
	int i;
	uval rc;
	uval ret[8];
	uval flags = 0;
	uval idx;
	union pte *ptep = (union pte *)ret;
	uval vsid = get_vsid(eaddr);
	uval lp = get_log_pgsize(pi, l, ls);
	
	idx = NUM_PTES_IN_PTEG * get_pteg(eaddr, vsid, lp);

	for (i = 0; i < NUM_PTES_IN_PTEG; i++) {
		rc = hcall_read(ret, flags, idx + i);

		assert((rc == H_Success),
		       "H_READ: FAILURE: returned %ld\n", rc);

		if (ptep->bits.v == 1) {
			union pte p;

			pte_init(&p, eaddr, 0, vsid, 0, l, ls);

			if (p.bits.avpn == ptep->bits.avpn) {
				return (idx + i);
			}
		}
	}
	return -1;
}


int
pte_enter(struct partition_info *pi, uval eaddr, uval raddr,
	  uval flags, int bolted, uval8 l, uval8 ls)
{
	union pte pte; /* temporary PTE */
	uval ptex;
	sval rc;
	uval ret2[2];
	uval hflags = 0;
	uval vsid  = get_vsid(eaddr);
	uval myflags = flags;

	/* find empty PTE in the correct primary PTEG */

	pte_init(&pte, eaddr, raddr, vsid, bolted, l, ls);

	pte.bits.w = (PAGE_WIMG_W & myflags) ? 1: 0;
	pte.bits.i = (PAGE_WIMG_I & myflags) ? 1: 0;
	pte.bits.m = (PAGE_WIMG_M & myflags) ? 1: 0;
	pte.bits.g = (PAGE_WIMG_G & myflags) ? 1: 0;

	rc = pte_find_free(pi, eaddr, l, ls);
	if (rc == -1) {
		assert(0, "we don't invalidate pte's yet\n");
		return -1;
	}
	ptex = (uval)rc;

#ifdef DEBUG_PTE_ENTER
	hprintf("H_ENTER: ptex = %ld\n", ptex);
#endif
	
	rc = hcall_enter(ret2, hflags, (uval)ptex,
			 pte.words.vsidWord, pte.words.rpnWord);
	if (rc != H_Success) {
		hprintf("H_ENTER: FAILURE: returned %ld\n", rc);
		return rc;
	}
	if (ret2[0] != ptex) {
		/* hypervisor gave us another ptex, which is ok, if it
		 * is not then we should use the H_EXACT flag and deal
		 * with the failure */
		/* interesting that this should happed though so lets
		 * log it */
		hprintf("H_ENTER: placed PTE in %lu when asked for %lu\n",
			ret2[0], ptex);
		ptex = ret2[0];
	}

#ifdef DEBUG_PTE_ENTER
	{
		uval ret8[8];

		rc = hcall_read(ret8, 0ULL, ptex);
		if (rc) {
			hprintf("H_READ: FAILURE: returned %ld\n", rc);
			return -1;
		}
		if (ret8[0] != pte.words.vsidWord ||
		    ret8[1] != pte.words.rpnWord) {
			hprintf("enter/read[%ld]: 0x%lx, 0x%lx no match\n",
				ptex, ret8[0], ret8[1]);
		} else {
			hprintf("enter/read[%ld]: 0x%lx, 0x%lx yes match\n",
				ptex, ret8[0], ret8[1]);
		}
	}
#endif /* DEBUG_PTE_ENTER */
	return ptex;
}

int
pte_remove(int ptex)
{
	uval ret2[2];
	sval rc;

	rc = hcall_remove(ret2, 0, ptex, 0);
	if (rc != H_Success) {
		hprintf("unable to find PTE in slot 0x%x\n", ptex);
	}

	return 0;
}

static inline uval
get_ioea(uval size)
{
	/* FIXME: should lock or get an atomic incrementer */
	static uval ioea = 0xeULL << ((sizeof (uval) * 8) - 4);
	uval ret;

	if (ioea == 0xeULL << ((sizeof (uval) * 8) - 4)) {
		slb_insert(ioea, 0, 0);
	}

	ret = ioea;
	ioea += size;
	return ret;
}

/* this sure looks dangerous */
void
iounmap_ptx(int ptx_num, int *ptx_list)
{
	uval ret2[2];
	uval flags = 0;
	int i;

	for (i = 0; i < ptx_num; i++) {
		/* yes this a blind removal */
		uval rc;
		rc = hcall_remove(ret2, flags, ptx_list[i], 0);
		assert((rc == H_Success), "hcall_remove() failed\n");
	}
}

void *
iomap_ptx(struct partition_info *pi,
	  uval raddr, uval size,  int bolted, uval8 l, uval8 ls,
	  int ptx_num, int *ptx_list)
{
	uval roff;
	uval eaddr;
	uval off;
	uval lp = get_log_pgsize(pi, l, ls);
	uval pgsize = 1 << lp;

	roff = raddr & PGMASK;
	size = PGALIGN(raddr + size) - roff;

	if (size == 0) {
		return NULL;
	}

	eaddr = get_ioea(size);

	for (off = 0; off < size; off += pgsize) {
		int rc = pte_enter(pi, eaddr + off, roff + off,
				   PAGE_WIMG_G | PAGE_WIMG_I,
				   bolted, l, ls);

		assert((rc >= 0), "pte_enter(): %d failed\n", rc);

		if (ptx_num > 0) {
			ptx_list[ptx_num - 1] = rc;
			--ptx_num;
		}
	}
	return (void *)(eaddr + (raddr & ~PGMASK));
}

void *
iomap(struct partition_info *pi,
      uval raddr, uval size, int bolted, uval8 l, uval8 ls)
{
	return iomap_ptx(pi, raddr, size, bolted, l, ls, 0, NULL);
}

int
tlb_enter(struct partition_info *pi, uval eaddr, uval raddr,
	  uval flags, uval8 l, uval8 ls)
{
	uval pvpn  = 0;
	uval vm_map_flags = 0;
	union ptel ptel;
	uval vsid   = get_vsid(eaddr);
	sval rc;

	(void)pi;
	
	ptel.word = 0;
	vm_map_flags |= H_VM_MAP_INSERT_TRANSLATION;
	
	/* setup the PTEL */
	
	ptel.bits.rpn = (raddr >> RPN_SHIFT);
	
	if (l) {
		uval8 lpbits = 0;
		
		vm_map_flags |= H_VM_MAP_LARGE_PAGE;
		
		while (ls--) {
			lpbits <<= 1;
			lpbits |= 1;
		}
		ptel.bits.rpn |= lpbits;
	}
		
	ptel.bits.r = 0; /* referenced */
	ptel.bits.c = 1; /* changed */
	ptel.bits.pp0 = PP_RWRW & 0x1UL;
	ptel.bits.pp1 = (PP_RWRW >> 1) & 0x3UL;
	ptel.bits.w = (PAGE_WIMG_W & flags) ? 1: 0;
	ptel.bits.i = (PAGE_WIMG_I & flags) ? 1: 0;
	ptel.bits.m = (PAGE_WIMG_M & flags) ? 1: 0;
	ptel.bits.g = (PAGE_WIMG_G & flags) ? 1: 0;
	
	/* find the PVPN */
	
	/* put the upper 4 bits of the 52 bit VSID at the end
	 * of the flags */
	vm_map_flags |= ((vsid >> 48) & 0xF);
	
	/* put the lower 48 bits of the 52 bit VSID into the
	 * uppder 48 bits of the PVPN */
	pvpn = ((vsid << 16)  & 0xFFFFFFFFFFFF0000);
	/* put the bits 36 - 451 of the EA into the lower 16
	 * bits of the PVPN */ 
	pvpn |= ((eaddr >> 12) & 0x000000000000FFFF);
	
	
	hprintf("VM_MAP: flags 0x%lx pvpn 0x%lx ptel 0x%llx\n",
		vm_map_flags, pvpn, ptel.word);
	
	rc = hcall_vm_map(NULL, vm_map_flags, pvpn, ptel.word);
	
	if (rc != H_Success) {
		hprintf("VM_MAP: FAILURE: returned %ld\n", rc);
	}
	
	return rc;
}
