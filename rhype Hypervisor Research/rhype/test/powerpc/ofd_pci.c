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
#include <test.h>
#include <lib.h>
#include <ofd.h>
#include <hcall.h>

static const char ranges[] = "ranges";

static uval
ofd_pci_phys_ranges(struct of_pci_range64_s *r64, uval num, uval cells,
		    struct range *r, uval64 *mmin, uval64 *mmax)
{
	uval i;
	uval64 max = 0ULL;
	uval64 min = ~0ULL;
	uval m = 0;

	/* precalc raddrs and rsizes */
	for (i = 0; i < num; i++) {
		if (cells == 2) {
			r[i].base = r64[i].opr_phys_hi;
			r[i].base <<= 32;
			r[i].base |= r64[i].opr_phys_lo;

			r[i].size = r64[i].opr_size_hi;
			r[i].size <<= 32;
			r[i].size |= r64[i].opr_size_lo;
		} else {
			struct of_pci_range32_s *r32;

			r32 = (struct of_pci_range32_s *)r64;
			r[i].base = r32[i].opr_phys;
			r[i].size = r32[i].opr_size;
		}
		r[i].end = r[i].base + r[i].size;

		if (r64[i].opr_addr.opa_hi.bits.opa_s > 0) {
			++m;
			max = MAX(max, r[i].end);
			min = MIN(min, r[i].base);
		}
	}
	if (m > 0) {
		*mmax = max;
		*mmin = min;
	}

	return 1;
}


static uval
ofd_pci_alloc_ranges(struct of_pci_range64_s *r64,
		     uval num, uval cells, uval lpid, uval buid)
{
	struct range *r;
	uval64 mmin = 0;
	uval64 mmax = 0;

	if (lpid == H_SELF_LPID) return 1;

	r = alloca(sizeof (*r) * num);
	ofd_pci_phys_ranges(r64, num, cells, r, &mmin, &mmax);

	assert(mmax > 0, "no PCI memory regions?\n");

	return ofd_pci_range_register(r64, num, cells, lpid, buid,
				      r, mmax, mmin);
}

static uval
ofd_pci_adjust_addr(void *m, ofdn_t n,
	       struct of_pci_range64_s *ro,
	       struct of_pci_range64_s *rn,
	       uval len)
{
	static const char aa_str[] = "assigned-addresses";
	sval sz;
	struct of_pci_addr_range64_s *aa;
	uval num;
	uval i;

	sz = ofd_getproplen(m, n, aa_str);
	assert(sz > 0, "No %s\n", aa_str);

	aa = alloca(sz);
	sz = ofd_getprop(m, n, aa_str, aa, sz);
	assert(sz > 0, "No %s\n", aa_str); // redundant, I know
	assert((sz % sizeof (*aa)) == 0, "bad size\n");

	num = sz / sizeof(*aa);
	for (i = 0; i < num; i++) {
		uval64 addr;
		uval64 end;
		uval j;
		union of_pci_hi phi;

		phi = aa[i].opr_addr.opa_hi;

		if (phi.bits.opa_s == 0) {
			/* The spec says:
			 * The type code shall not be '00'
			 * (Configuration Space)
			 */
			hprintf("%s: %u/%s has bad type code in %luth entry\n",
				__func__,
				n,
				aa_str,
				i);
		}
		if (phi.bits.opa_n == 1) {
			/* The spec says:
			 * The 'n' bit of each phys-addr shall be set
			 * to 1, indicating that the address is
			 * absolute (within the PCI domain's address
			 * space), not relative to the start of a
			 * relocatable region.
			 */
			hprintf("%s: %u/%s n bit must be zero in the"
				"%luth entry\n",
				__func__,
				n,
				aa_str,
				i);
		}
		addr = aa[i].opr_addr.opa_mid;
		addr <<= 32;
		addr |= aa[i].opr_addr.opa_lo;
		end = aa[i].opr_size_hi;
		end <<= 32;
		end |= aa[i].opr_size_lo;
		end += addr;

		for (j = 0; j < len; j++) {
			uval64 raddr;
			uval64 rsize;
			uval64 naddr;

			raddr = ro[j].opr_phys_hi;
			raddr <<= 32;
			raddr |= ro[j].opr_phys_lo;
			rsize = ro[j].opr_size_hi;
			rsize <<= 32;
			rsize |= ro[j].opr_size_lo;

			if (!range_contains(raddr, rsize, addr)) {
				continue;
			}

			/* addr..end is between raddr and raddr+rsize */
			assert(range_contains(raddr, rsize, end),
			       "range overlaps\n");

			naddr = rn[j].opr_phys_hi;
			naddr <<= 32;
			naddr += rn[j].opr_phys_lo;

			addr -= raddr - naddr;

			/* update */
			aa[i].opr_addr.opa_mid = addr >> 32 & 0xffffffffUL;
			aa[i].opr_addr.opa_lo = addr & 0xffffffffUL;
			break;
		}
	}
	ofd_setprop(m, n, aa_str, aa, sz);
	return 1;
}

uval64
ofd_pci_addr(uval mem)
{
	void *m = (void *)mem;
	ofdn_t n;
	uval32 cells = 1;
	uval32 size_cells = 1;
	uval32 sz[8];		/* should be enough */
	uval len;
	uval64 pci_addr;
	/*uval64 pci_laddr;*/
	uval pci_config_size;
	sval rc;

	n = ofd_node_find(m, "pci");
	if (n <= 0) {
		hprintf("WARN: no PCI bus available\n");
		return 0;
	}

	ofdn_t parent = ofd_node_parent(m, n);

	rc = ofd_getcells(m, parent, &cells, &size_cells);
	assert(rc >= 0, "Failed to get cell sizes\n");
	assert(cells == 2, "Need #address-cells==2\n");

	len = ofd_getprop(m, n, "reg", &sz, sizeof (sz));

	assert(len == sizeof(uval32) * (cells + size_cells),
	       "no PCI config address available\n");
	if (cells == 1) {
		pci_addr = (uval64)sz[0];
	} else {
		pci_addr = *(uval64 *)sz;
	}

	if (size_cells == 1) {
		pci_config_size = (uval64)sz[cells];
	} else {
		pci_config_size = *(uval64 *)(sz + cells);
	}

	if (pci_addr == 0) {
		return pci_addr;
	}


	uval buid = 0;
	rc = hcall_pci_config(NULL, SET|CREATE, buid, 0,0,0);
	assert(rc == H_Success, "Failed defining PCI bus\n");

	uval32 ioid;
	len = ofd_getprop(m, n, "ioid", &ioid, sizeof (ioid));
	if (len != sizeof (ioid)) {
		ioid = 0;
	}

	rc = hcall_pci_config(NULL, SET|IOID, buid, ioid, 0,0);
	assert(rc == H_Success, "Failed setting ioid\n");

#if 0
	/* Should the actuall pci controller (as per "reg") be accessible ?*/
	/* This is even more dubious*/
	//rc = hcall_pci_config(NULL, SET|CFG_SPC, buid, pci_addr,
	//			pci_config_size, 0);
	//assert(rc == H_Success, "Failed setting cfg spc\n");
	rc = hcall_mem_define(rets, MMIO_ADDR, pci_addr, pci_config_size);
	assert(rc == H_Success, "Failed defining PCI config space addrs\n");

	pci_laddr = rets[0];

	if (cells == 1) {
		sz[0] = pci_laddr;
	} else {
		*(uval64)sz = pci_laddr;
	}

	ofd_setprop(m, n, "reg", &sz, cells+size_cells);
#endif

	struct dma_window {
		uval32 liobn;
		uval32 ignore;
		uval64 phys;
		uval32 ignore2;
		uval32 size;
	} dma_win;


	len = ofd_getprop(m, n, "ibm,dma-window", &dma_win, sizeof(dma_win));
	if (len == sizeof(dma_win)) {
		rc = hcall_pci_config(NULL, SET|DMA_WIN, buid,
				      dma_win.liobn, dma_win.phys,
				      dma_win.size);

		assert(rc == H_Success, "Failed setting DMA window\n");

	}
	/* FIXME assumes pci64 */
	struct of_pci_range64_s *r;
	struct of_pci_range64_s *rold;
	sval rsz;

	rc = ofd_getcells(m, n, &cells, &size_cells);
	assert(rc >= 0, "Failed to get cell sizes\n");

	/* there is no spec'd limit to ranges */
	len = ofd_getproplen(m, n, ranges);
	assert(len > 0, "No PCI ranges\n");
	assert(len % sizeof(*r) == 0, "PCI ranges is worng size\n");

	r = alloca(len);
	rold = alloca(len);

	rsz = ofd_getprop(m, n, ranges, r, len);

	memcpy(rold, r, rsz);

	len = rsz / sizeof (*r);

	if (ofd_pci_alloc_ranges(r, len, size_cells, 0, buid) == 0) {
		/* problem with the PCI ranges, must inhibit the IO Host */
		iohost_lpid = 1;
	}

	ofd_setprop(m, n, ranges, r, rsz);

	ofdn_t aa;
	aa = ofd_node_find_by_prop(m, n, "assigned-addresses", NULL, 0);
	while (aa > 0) {
		ofd_pci_adjust_addr(m, aa, rold, r, len);
		aa = ofd_node_find_next(m, aa);
	}

	return pci_addr;
}

uval
config_hba(void *m, uval lpid)
{
	sval rc;
	ofdn_t n;
	ofdn_t aa;
	struct of_pci_range64_s *r64;
	struct of_pci_range64_s *rold;
	sval rsz;
	uval32 cells;
	uval32 size_cells;
	sval len;

	n = ofd_node_find(m, "pci");
	if (n <= 0)
		return 0;

	uval buid = 0; /* FIXME: get the right bus id */
	/* Permit h_pci_config_{read,write} calls on bus id */
	rc = hcall_pci_config(NULL, SET|GRANT, buid, lpid, 0,0);
	assert(rc == H_Success,
	       "PCI grant: lpid[0x%lx] failed\n",
	       lpid);


	rc = ofd_getcells(m, ofd_node_parent(m,n), &cells, &size_cells);
	assert(rc >= 0, "Failed to get cell sizes\n");

	/* there is no spec'd limit to ranges */
	len = ofd_getproplen(m, n, ranges);
	assert(len > 0, "No PCI ranges\n");
	r64 = alloca(len);
	rold = alloca(len);
	rsz = ofd_getprop(m, n, ranges, r64, len);
	if (cells == 2) {
		len = rsz / sizeof (struct of_pci_range64_s);
	} else {
		len = rsz / sizeof (struct of_pci_range32_s);
	}

	memcpy(rold, r64, rsz);

	ofd_pci_alloc_ranges(r64, len, cells, lpid, 0);
	ofd_setprop(m, n, ranges, r64, rsz);

	aa = ofd_node_find_by_prop(m, n, "assigned-addresses", NULL, 0);
	while (aa > 0) {
		ofd_pci_adjust_addr(m, aa, rold, r64, len);
		aa = ofd_node_find_next(m, aa);
	}


	return 1;
}
