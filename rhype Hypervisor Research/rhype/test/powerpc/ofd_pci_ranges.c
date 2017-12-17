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

uval
ofd_pci_range_register(struct of_pci_range64_s *r64,
		       uval num, uval cells, uval lpid, uval buid,
		       struct range *r,
		       uval mmax __attribute__ ((unused)),
		       uval mmin __attribute__ ((unused)))
{
	uval i;
	sval rc;
	uval rets[1];

	/* register config space(s) and adjust the tree */
	for (i = 0; i < num; i++) {
		uval scode;
		uval64 base;

		scode = r64[i].opr_addr.opa_hi.bits.opa_s;
		if (lpid == 0) {
			if (scode == 0) {
				rc = hcall_pci_config(NULL, SET|CFG_SPC, buid,
						      r[i].base, r[i].size, 0);
				assert(rc == H_Success,
				       "Failed setting cfg spc\n");
			}

			rc = hcall_mem_define(rets, MMIO_ADDR,
					      r[i].base, r[i].size);

			assert(rc == H_Success,
			       "hcall_mem_define: failed\n");
		} else {
			rc = hcall_resource_transfer(rets, MMIO_ADDR,
						     0, r[i].base,
						     r[i].size, lpid);
			assert(rc == H_Success,
			       "hcall_resource_transfer: failed\n");
		}

		hprintf("PCI: RA 0x%016llx -> LA 0x%016lx [0x%llx]\n",
			r[i].base, rets[0], r[i].size);

		base = rets[0];

		if (cells == 2) {
			r64[i].opr_phys_hi = base >> 32;
		} else {
			assert((base >> 32) == 0,
			       "64bit addr for 32bit PCI\n");
			r64[i].opr_phys_hi = 0;
		}
		r64[i].opr_phys_lo = base & 0xffffffff;
	}
	return 1;
}
