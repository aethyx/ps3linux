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
#include <hba.h>
#include <lib.h>

#define PCI_DEBUG

sval
h_pci_config_init(struct cpu_thread *thread, uval buid, uval caddr,
		  uval iopt_size, uval devid)
{
	struct hba *hba;
	uval owner = thread->cpu->os->po_lpid;

	hba = hba_get(buid);

	if (hba == NULL) {
#ifdef PCI_DEBUG
		hprintf("PCI bus ID: 0x%lx\n", buid);
#endif
		return H_Parameter;
	}

	if (owner != hba->hba_owner_lpid) {
#ifdef PCI_DEBUG
		hprintf("LPAR[0x%lx] does not own bus ID: 0x%lx\n",
			owner, buid);
#endif
		return H_Parameter;
	}

	hba->hba_config_addr = caddr;
	hba->hba_devid = devid;
	/* FIXME: need to do more here */
	hba->hba_dma_base = 0;
	hba->hba_dma_size = iopt_size;

	return_arg(thread, 1, buid);
	return_arg(thread, 2, hba->hba_dma_base);
	return_arg(thread, 3, hba->hba_dma_size);

	return H_Success;
}
