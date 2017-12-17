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
#include <hba.h>
#include <rtas.h>
#include <lib.h>
#include <of_devtree.h>
#include <lpar.h>

static sval32 hba_read_pci_config = -1;
static sval32 hba_write_pci_config = -1;
static sval32 hba_ibm_read_pci_config = -1;
static sval32 hba_ibm_write_pci_config = -1;

static uval
rtas_pci_config_read(uval addr, uval buid, uval size, uval *val)
{
	struct rtas_args_s *r;
	uval nargs;

	r = alloca(sizeof (*r) + sizeof (r->ra_args[0]) * 6);

	if (buid == 0 && hba_read_pci_config > 0) {
		nargs = 2;
		r->ra_token = hba_read_pci_config;
		r->ra_nargs = nargs;
		r->ra_nrets = 2;
		r->ra_args[0] = addr;
		r->ra_args[1] = size;
	} else {
		uval64 b = buid;

		if (hba_ibm_read_pci_config <= 0) {
			return 0;
		}
		nargs = 4;
		r->ra_token = hba_ibm_read_pci_config;
		r->ra_nargs = nargs;
		r->ra_nrets = 2;
		r->ra_args[0] = addr;
		r->ra_args[1] = (b >> 32) & 0xffffffffUL;
		r->ra_args[2] = buid & 0xffffffffUL;
		r->ra_args[3] = size;
	}

	rtas_call(r);
	if (r->ra_args[nargs] == RTAS_SUCCEED) {
		*val = r->ra_args[nargs + 1];
		return size;
	}
	return 0;
}

uval
hba_config_read(uval lpid, uval addr, uval buid, uval size, uval *val)
{

	struct hba *hba = hba_get(buid);

	if (!hba || !hba_owner(hba, lpid)) {
		return 0;
	}
	return rtas_pci_config_read(addr, buid, size, val);
}

static uval
rtas_pci_config_write(uval addr, uval buid, uval size, uval val)
{
	struct rtas_args_s *r;
	uval nargs;
	sval ret;

	r = alloca(sizeof (*r) + sizeof (r->ra_args[0]) * 6);

	if (buid == 0 && hba_write_pci_config > 0) {
		nargs = 3;
		r->ra_token = hba_write_pci_config;
		r->ra_nargs = nargs;
		r->ra_nrets = 1;
		r->ra_args[0] = addr;
		r->ra_args[1] = size;
		r->ra_args[2] = val;
	} else {
		uval64 b = buid;

		if (hba_ibm_write_pci_config <= 0) {
			return 0;
		}
		nargs = 5;
		r->ra_token = hba_ibm_write_pci_config;
		r->ra_nargs = nargs;
		r->ra_nrets = 1;
		r->ra_args[0] = addr;
		r->ra_args[1] = (b >> 32) & 0xffffffffUL;
		r->ra_args[2] = buid & 0xffffffffUL;
		r->ra_args[3] = size;
		r->ra_args[4] = val;
	}

	ret = rtas_call(r);
	if (ret == RTAS_SUCCEED && r->ra_args[nargs] == RTAS_SUCCEED) {
		return size;
	}
	return 0;
}

uval
hba_config_write(uval lpid, uval addr, uval buid, uval size, uval val)
{
	struct hba *hba = hba_get(buid);

	if (!hba || !hba_owner(hba, lpid)) {
		return 0;
	}
	return rtas_pci_config_write(addr, buid, size, val);
}

extern void __hba_init(void);

sval
hba_init(uval ofd)
{
	void *m = (void *)ofd;
	ofdn_t r;
	sval p;

	__hba_init();

	if (rtas_entry == 0) {
		return H_Hardware;
	}

	/*
	 * check to see if standard PCI RTAS calls are supported
	 */
	r = ofd_node_find(m, "/rtas");
	if (r <= 0) {
		hprintf("No /rtas node found in OF\n");
		return H_Hardware;
	}

	p = ofd_getprop(m, r, "read-pci-config",
			&hba_read_pci_config, sizeof (hba_read_pci_config));
	p = ofd_getprop(m, r, "write-pci-config",
			&hba_write_pci_config, sizeof (hba_write_pci_config));

	p = ofd_getprop(m, r, "ibm,read-pci-config",
			&hba_ibm_read_pci_config,
			sizeof (hba_ibm_read_pci_config));
	p = ofd_getprop(m, r, "write-pci-config",
			&hba_ibm_write_pci_config,
			sizeof (hba_ibm_write_pci_config));

	if (hba_read_pci_config <= 0 && hba_ibm_read_pci_config <= 0) {
		hprintf("There are no \"read-pci-config\" methods");
		return H_Hardware;
	}
	if (hba_write_pci_config <= 0 && hba_ibm_write_pci_config <= 0) {
		hprintf("There are no \"write-pci-config\" methods");
		return H_Hardware;
	}
#ifdef DEBUG
	if (hba_read_pci_config >= 0) {
		uval val;
		uval sz;

		hputs("Probing PCI bus 0 using RTAS\n");
		sz = rtas_pci_config_read(0x0, 0, 2, &val);
		if (sz == 2) {
			hprintf("PCI[0]: Vendor ID: 0x%lx\n", val);
		} else {
			hprintf("rtas_pci_config_read(0x0) returned: 0x%lx\n",
				sz);
		}

		sz = rtas_pci_config_read(0x2, 0, 2, &val);
		if (sz == 2) {
			hprintf("PCI[0]: Device ID: 0x%lx\n", val);
		} else {
			hprintf("rtas_pci_config_read(0x2) returned: 0x%lx\n",
				sz);
		}

		sz = rtas_pci_config_read(0xe, 0, 1, &val);
		if (sz == 2) {
			hprintf("PCI[0]: Header Type ID: 0x%lx\n", val);
		} else {
			hprintf("rtas_pci_config_read(0xe) returned: 0x%lx\n",
				sz);
		}
	}
#endif

	return H_Success;
}
