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
#include <hcall.h>
#include <eic.h>
#include <hype.h>
#define PCI_DEBUG

sval
h_pci_config(struct cpu_thread *thread, uval command, uval buid, uval arg1,
	     uval arg2, uval arg3)
{
	uval cmd = command & ~SET;
	uval set = command & SET;
	sval ret = 0;
	struct hba *hba;

	if (thread->cpu->os != ctrl_os) {
		return H_Permission;
	}

	lock_acquire(&hba_lock);
	if (cmd == CREATE) {
		assert(set, "Don't know how to delete pci busses\n");
		ret = hba_define(buid);
		goto abort;
	};

	hba = hba_get(buid);
	if (hba == NULL) {
		ret = H_Parameter;
		goto abort;
	}

	switch (cmd) {
	case IOID:
		hba->ioid = arg1;
		break;
	case CFG_SPC:
		hba->config_space.start = arg1;
		hba->config_space.size = arg2;
		break;
	case DMA_WIN:
		hba->dma_win.liobn = arg1;
		hba->dma_win.start = arg2;
		hba->dma_win.size = arg3;
		break;
	case GRANT:
		{
			struct os *client = os_lookup(arg1);
			if (!client) {
				ret = H_Parameter;
				break;
			}
			int i = 0;

			for (; hba->users[i] != 0
			     && hba->users[i] != arg1 && i < MAX_USERS; ++i) ;
			if (i == MAX_USERS) {
				ret = H_Parameter;
			} else {
				hba->users[i] = client->po_lpid;
			}
		}
	}

/* *INDENT-OFF* */
abort:
/* *INDENT-ON* */
	lock_release(&hba_lock);
	return ret;
}
