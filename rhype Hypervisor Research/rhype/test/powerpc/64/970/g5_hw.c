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

#include <types.h>
#include <lib.h>
#include <openfirmware.h>
#include <of_devtree.h>
#include <ofd.h>
#include <test.h>
#include <mmu.h>
#include <hcall.h>

void ofd_platform_probe(void* ofd)
{
	(void)ofd;
	sval rc;
	uval rets[1];
	rc = hcall_mem_define(rets, MMIO_ADDR, 0xf2000000, 0x02000000);
	assert(rc == H_Success, "Can't define HT region\n");

	hprintf("Defined HyperTransport MMIO: %lx\n", rets[0]);

	/* G5's have a little-endian openpic */
	ofd_openpic_probe(ofd, 1);


	/* Define the ibm,dma-window corresponding to a complete DART */
	uval32 ofd_pci_dma_window[] = { 0, 0, 0, 0, 0, 1<<31 };
	ofdn_t n = ofd_node_find(ofd, "pci");
	if (n > 0)
		ofd_prop_add(ofd, n, "ibm,dma-window",
			     &ofd_pci_dma_window, sizeof (ofd_pci_dma_window));

}
