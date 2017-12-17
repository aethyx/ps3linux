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

#include <lib.h>
#include <openfirmware.h>
#include <of_devtree.h>
#include <ofd.h>
#include <test.h>
#include <mmu.h>
#include <hcall.h>
#include <sim.h>
#include <objalloc.h>
#include <bitops.h>

extern ofdn_t ofd_xics_props(void *m);

extern void ofd_platform_config(struct partition_status* ps, void* ofd)
	__attribute__((weak));


#define CHUNK_SIZE        (1UL << LOG_CHUNKSIZE)

static ofdn_t
ofd_vdevice_vscsi_server(void *m, ofdn_t p, uval lpid, uval num)
{
	ofdn_t n = 0;
	static const char pathfmt[] = "/vdevice/v-scsi-host@%x";
	static const char name[] = "v-scsi-host";
	static const char compatible[] = "IBM,v-scsi-host";
	static const char device_type[] = "v-scsi-host";
	char path[sizeof (pathfmt) + 8 - 1];
	uval32 intr[2] = { /* source */ 0x0, /* +edge */ 0x0 };
	uval32 dma_sz = 8 * 1024 * 1024; /* (((0x80<< 12) >> 3) << 12) */
	uval32 dma[] = {
		/* server */
		/* liobn */ 0x0,
		/* phys  */ 0x0, 0x0,
		/* size  */ 0x0, 0x0,

		/* client */
		/* liobn */ 0x0,
		/* phys  */ 0x0, 0x0,
		/* size  */ 0x0, 0x0
	};
	uval i;
	struct crq_maintenance *cm;

	for (i = 0; i < num; i++) {
		uval server;
		uval32 val;

		cm = crq_create(lpid, ROLE_SERVER, dma_sz);
		if (cm != NULL) {
			server = cm->cm_liobn_server;
			dma[0] = server;
			dma[4] = cm->cm_dma_sz;
			dma[5] = cm->cm_liobn_client;
			dma[9] = cm->cm_dma_sz;
			intr[0] = server;

			val = server;
			snprintf(path, sizeof (path), pathfmt, val);
			n = ofd_node_add(m, p, path, sizeof (path));
			assert(n > 0, "failed to create node\n");

			if (n > 0) {
				ofd_prop_add(m, n, "name",
					     name, sizeof (name));
				ofd_prop_add(m, n, "device_type",
					     device_type,
					     sizeof (device_type));
				ofd_prop_add(m, n, "compatible",
					     compatible, sizeof (compatible));
				ofd_prop_add(m, n, "reg",
					     &val, sizeof (val));
				ofd_prop_add(m, n, "ibm,my-dma-window",
					     dma, sizeof (dma));
				ofd_prop_add(m, n, "interrupts",
					     intr, sizeof (intr));
				ofd_prop_add(m, n, "ibm,vserver", NULL, 0);

				val = 2;
				ofd_prop_add(m, n, "ibm,#dma-address-cells",
					     &val, sizeof (val));
				ofd_prop_add(m, n, "ibm,#dma-size-cells",
					     &val, sizeof (val));
			}
		}
	}
	return n;
}


static ofdn_t
ofd_vdevice_vty_server(void *m, ofdn_t p, uval num)
{
	ofdn_t n = 0;
	static const char pathfmt[] = "/vdevice/vty-server@%x";
	static const char name[] = "vty-server";
	static const char compatible[] = "hvterm2";
	static const char device_type[] = "serial-server";
	char path[sizeof (pathfmt) + 8 - 1];
	uval i;
	uval server;

	for (i = 0; i < num; i++) {
		server = vterm_create_server();
		assert(server > 0, "failed to create server\n");
		if (server > 0) {
			uval32 val;

			val = server;
			snprintf(path, sizeof (path), pathfmt, val);
			n = ofd_node_add(m, p, path, sizeof (path));
			assert(n > 0, "failed to create node\n");
			if (n > 0) {
				uval32 intr[2] = {
					/* source */ 0x0,
					/* +edge */ 0x0
				};

				ofd_prop_add(m, n, "ibm,vserver", NULL, 0);
				ofd_prop_add(m, n, "name",
					     name, sizeof (name));
				ofd_prop_add(m, n, "reg",
					     &val, sizeof (val));
				ofd_prop_add(m, n, "compatible",
					     compatible, sizeof (compatible));
				ofd_prop_add(m, n, "device_type",
					     device_type,
					     sizeof (device_type));
				intr[0] = server;
				ofd_prop_add(m, n, "interrupts",
					     intr, sizeof (intr));
			}
		}
	}
	return n;
}
			

static ofdn_t
ofd_vdevice_vty(void *m, ofdn_t p, struct partition_status *ps)
{
	ofdn_t n;
	static const char pathfmt[] = "/vdevice/vty@%lx";
	static const char name[] = "vty";
	static const char compatible[] = "hvterm1";
	static const char device_type[] = "serial";
	char path[sizeof (pathfmt) + 8 - 2];
	uval server;
	uval client;

	vterm_create(ps, &server, &client);

	snprintf(path, sizeof (path), pathfmt, client);
	n = ofd_node_add(m, p, path, sizeof (path));

	if (n > 0) {
		uval32 val32;

		val32 = client;
		ofd_prop_add(m, n, "name", name, sizeof (name));
		ofd_prop_add(m, n, "reg", &val32, sizeof (val32));
		ofd_prop_add(m, n, "compatible",
				compatible, sizeof (compatible));
		ofd_prop_add(m, n, "device_type",
				device_type, sizeof (device_type));
	}
	if (server == 0 && ps->lpid == iohost_lpid) {
		ofd_vdevice_vty_server(m, p, 64);
	}

	return n;
}

static ofdn_t
ofd_vdevice_llan(void *m, ofdn_t p, uval lpid)
{
	ofdn_t n;
	static const char pathfmt[] = "/vdevice/l-lan@%lx";
	static const char name[] = "l-lan";
	static const char compatible[] = "IBM,l-lan";
	static const char device_type[] = "network";
	char path[sizeof (pathfmt) + 8 - 2];
	uval32 val;
	uval64 mac = 0x02ULL << 40;	/* local address tag */
	uval32 intr[2] = { /* source */ 0x0, /* +edge */ 0x0 };
	uval32 dma_sz = 8 * 1024 * 1024; /* (((0x80<< 12) >> 3) << 12) */
	uval32 dma[5] = {
		/* liobn */ 0x0,
		/* phys  */ 0x0, 0x0,
		/* size  */ 0x0, 0x0
	};
	uval liobn;

	dma_sz = llan_create(dma_sz, lpid, &liobn);

	dma[0] = liobn;
	dma[4] = dma_sz;
	intr[0] = liobn;
	mac |= liobn;


	snprintf(path, sizeof (path), pathfmt, liobn);
	n = ofd_node_add(m, p, path, sizeof (path));

	if (n > 0) {
		ofd_prop_add(m, n, "name", name, sizeof (name));
		val = liobn;
		ofd_prop_add(m, n, "reg", &val, sizeof (val));
		ofd_prop_add(m, n, "compatible",
				compatible, sizeof (compatible));
		ofd_prop_add(m, n, "device_type",
				device_type, sizeof (device_type));

		val = 2;
		ofd_prop_add(m, n, "ibm,#dma-address-cells",
			     &val, sizeof (val));
		ofd_prop_add(m, n, "ibm,#dma-size-cells", &val, sizeof (val));

		val = 255;
		ofd_prop_add(m, n, "ibm,mac-address-filters",
			     &val, sizeof (val));

		ofd_prop_add(m, n, "ibm,vserver", NULL, 0);
		ofd_prop_add(m, n, "local-mac-address", &mac, sizeof (mac));
		ofd_prop_add(m, n, "mac-address", &mac, sizeof (mac));
		ofd_prop_add(m, n, "ibm,my-dma-window", dma, sizeof (dma));
		ofd_prop_add(m, n, "interrupts", intr, sizeof (intr));
	}
	return n;
}

static ofdn_t
ofd_vdevice(void *m, struct partition_status *ps)
{
	ofdn_t n;
	static const char path[] = "/vdevice";
	static const char name[] = "vdevice";
	static const char compatible[] = "IBM,vdevice";
	uval32 val;

	n = ofd_node_add(m, OFD_ROOT, path, sizeof (path));

	if (n > 0) {
		ofdn_t r;

		ofd_prop_add(m, n, "name", name, sizeof (name));
		val = 1;
		ofd_prop_add(m, n, "#address-cells", &val, sizeof (val));
		val = 0;
		ofd_prop_add(m, n, "#size-cells", &val, sizeof (val));
		ofd_prop_add(m, n, "compatible",
				compatible, sizeof (compatible));
		ofd_prop_add(m, n, "device_type", name, sizeof (name));
		ofd_prop_add(m, n, "interupt-controller", NULL, 0);

		/* add vty */
		r = ofd_vdevice_vty(m, n, ps);
		hprintf("vdevice r: %x\n",r );
		if (r > 0) {
			r = ofd_vdevice_llan(m, n, ps->lpid);
		}
		if (r > 0) {
			hprintf("VSCSI: lpid: 0x%lx iohost: 0x%lx\n",
				ps->lpid, iohost_lpid);
			if (ps->lpid == iohost_lpid) {
				r = ofd_vdevice_vscsi_server(m, n,
							     ps->lpid, 10);
			}
		}
		n = r;
	}
	return n;
}

static ofdn_t
ofd_root_props(void *m, uval lpid)
{
	ofdn_t n = OFD_ROOT;

	ofd_prop_add(m, n, "ibm,lpar-capable", NULL, 0);

	if (lpid == iohost_lpid) {
		/* force machine compatibility to be the real thing */
		static const char compat[] = MACHINE_NAME_STRING;
		ofd_prop_add(m, n, "compatible", compat, sizeof (compat));
	} else {
		/* force machine compatibility to be PSERIES_LPAR */
		static const char compat[] = "PowerPC-System";
		ofd_prop_add(m, n, "compatible", compat, sizeof (compat));
	}
	return n;
}

static ofdn_t
ofd_openprom_props(void *m)
{
	static const char path[] = "/openprom";
	static const char vernum[] = "IBM,rHypeOF0.1";
	ofdn_t n;

	n = ofd_node_find(m, path);
	if (n == 0) {
		n = ofd_node_add(m, OFD_ROOT, path, sizeof (path));
		ofd_prop_add(m, n, "name",
				&path[1], sizeof (path) - 1);
	}
	/* I want to override */
	ofd_prop_add(m, n, "model", vernum, sizeof(vernum));
	ofd_prop_add(m, n, "ibm,fw-vernum_encoded", vernum, sizeof(vernum));
	ofd_prop_add(m, n, "relative-addressing", NULL, 0);
	return n;

}

static ofdn_t
ofd_aliases_props(void *m)
{
	static const char path[] = "/aliases";
	static const char vty[] = "/vdevice/vty@0";
	static const char net[] = "/vdevice/l-lan@1";
	ofdn_t n;

	n = ofd_node_find(m, path);
	if (n == 0) {
		n = ofd_node_add(m, OFD_ROOT, path, sizeof (path));
		ofd_prop_add(m, n, "name",
				&path[1], sizeof (path) - 1);
	}
	ofd_prop_add(m, n, "screen", vty, sizeof(vty));
	ofd_prop_add(m, n, "net", net, sizeof(net));
	ofd_prop_add(m, n, "network", net, sizeof(net));
	return n;
}

static ofdn_t
ofd_options_props(void *m)
{
	static const char path[] = "/options";
	static const char boot[] = "true";
	ofdn_t n;

	n = ofd_node_find(m, path);
	if (n == 0) {
		n = ofd_node_add(m, OFD_ROOT, path, sizeof (path));
		ofd_prop_add(m, n, "name",
				&path[1], sizeof (path) - 1);
	}
	ofd_prop_add(m, n, "auto-boot?", boot, sizeof(boot));
	return n;
}

static ofdn_t
ofd_cpus_props(void *m, struct partition_status *ps)
{
	static const char path[] = "/cpus";
	static const char cpu[] = "cpu";
	uval32 val = 1;
	ofdn_t n;
	ofdn_t c;
	static uval32 ibm_pft_size[] = { 0x0, 0x0 };

	n = ofd_node_find(m, path);
	if (n == 0) {
		n = ofd_node_add(m, OFD_ROOT, path, sizeof (path));
		ofd_prop_add(m, n, "name",
				&path[1], sizeof (path) - 1);
	}
	ofd_prop_add(m, n, "#address-cells", &val, sizeof(val));
	ofd_prop_add(m, n, "#size-cells", &val, sizeof(val));
	ofd_prop_add(m, n, "smp-enabled", NULL, 0);

#ifdef HV_EXPOSE_PERFORMANCE_MONITOR
	ofd_prop_add(m, n, "performance-monitor", NULL, 0);
#endif

	c = ofd_node_find_by_prop(m, n, "device_type", cpu, sizeof (cpu));
	//assert(c > 0, "can't get first processor\n");
	while (c > 0) {
		ibm_pft_size[1] = ps->log_htab_bytes;
		ofd_prop_add(m, c, "ibm,pft-size",
			     ibm_pft_size, sizeof (ibm_pft_size));

		/* FIXME: we don't sleep to good yet so if on simulator lie
		 * about the clock speed */
		if (onsim()) {
			val = 100000000;
			ofd_prop_add(m, c,
				     "clock-frequency", &val, sizeof(val));
			ofd_prop_add(m, c,
				     "timebase-frequency", &val, sizeof(val));
		}

		/* FIXME: Check the the "l2-cache" property who's
		 * contents is an orphaned phandle? */

		ofd_per_proc_props(m, c, ps->lpid);
		c = ofd_node_find_next(m, c);

		/* Since we are not MP yet we can prune the rest of the CPUs */
		while (c > 0) {
			ofdn_t nc;

			nc = ofd_node_find_next(m, c);
			ofd_node_prune(m, c);

			c = nc;
		}
	}

	ofd_proc_props(m, ps->lpid);
	return n;
}

static void
ofd_openpic_owner_props(void *m, uval lpid)
{
	static const char name[] = "platform-open-pic";
	uval64 addr = ~0ULL;
	ofd_getprop(m, OFD_ROOT, name, &addr, sizeof(addr));
	if (addr == ~0ULL) return;

	sval rc = H_Success;
	uval rets[5];

	if (lpid != H_SELF_LPID) {
		rc = hcall_resource_transfer(rets, MMIO_ADDR,
					     0, (uval)addr, 0x40000, lpid);
		assert(rc == H_Success, "hcall failure: %lx\n", rc);
		if (rc == H_Success) {
			ofd_setprop(m, OFD_ROOT, name, rets, sizeof(rets));
		}

	}

	rc = hcall_eic_config(rets, CTL_LPAR, lpid, 0, 0, 0);
	assert(rc == H_Success, "hcall failure: %lx\n", rc);
}

void
ofd_openpic_probe(void *m, uval little_endian)
{
	static const char name[] = "platform-open-pic";
	uval64 addr = ~0ULL;
	ofd_getprop(m, OFD_ROOT, name, &addr, sizeof(addr));
	if (addr == ~0ULL) return;

	sval rc;
	uval rets[5];

	rc = hcall_mem_define(rets, MMIO_ADDR, (uval)addr, 0x40000);

	hprintf("OpenPIC at: %lx %llx %lx\n", rc, addr, rets[0]);

	assert(rc == H_Success, "hcall failure: %lx\n", rc);

	if (rc == H_Success) {
		ofd_setprop(m, OFD_ROOT, name, rets, sizeof(rets));
	}

	rc = hcall_eic_config(rets, CONFIG, (uval)addr, little_endian, 0, 0);
	assert(rc == H_Success, "hcall failure: %lx\n", rc);
}


ofdn_t
ofd_xics_props(void *m)
{
	ofdn_t n;
	static const char path[] = "/interrupt-controller";
	static const char compat[] = "IBM,ppc-xicp";
	static const char model[] = "IBM, BoaC, PowerPC-PIC, 00";
	static const char dtype[] =
		"PowerPC-External-Interrupt-Presentation";
	/*
	 * I don't think these are used for anything but linux wants
	 * it.  I seems to describe some per processor location for
	 * IPIs but that is a complete guess.
	 */
	static const uval32 reg[] = {
		0x000003e0, 0x0f000000, 0x00000000, 0x00001000,
		0x000003e0, 0x0f001000, 0x00000000, 0x00001000,
		0x000003e0, 0x0f002000, 0x00000000, 0x00001000,
		0x000003e0, 0x0f003000, 0x00000000, 0x00001000,
		0x000003e0, 0x0f004000, 0x00000000, 0x00001000,
		0x000003e0, 0x0f005000, 0x00000000, 0x00001000,
		0x000003e0, 0x0f006000, 0x00000000, 0x00001000,
		0x000003e0, 0x0f007000, 0x00000000, 0x00001000,
		0x000003e0, 0x0f008000, 0x00000000, 0x00001000,
		0x000003e0, 0x0f009000, 0x00000000, 0x00001000,
		0x000003e0, 0x0f00a000, 0x00000000, 0x00001000,
		0x000003e0, 0x0f00b000, 0x00000000, 0x00001000,
		0x000003e0, 0x0f00c000, 0x00000000, 0x00001000,
		0x000003e0, 0x0f00d000, 0x00000000, 0x00001000,
		0x000003e0, 0x0f00e000, 0x00000000, 0x00001000,
		0x000003e0, 0x0f00f000, 0x00000000, 0x00001000,
	};

	n = ofd_node_find(m, path);
	if (n == 0) {
		n = ofd_node_add(m, OFD_ROOT, path, sizeof (path));
		ofd_prop_add(m, n, "name",
				&path[1], sizeof (path) - 1);
	}
	ofd_prop_add(m, n, "built-in", NULL, 0);
	ofd_prop_add(m, n, "compatible", compat, sizeof(compat));
	ofd_prop_add(m, n, "device_type", dtype, sizeof(dtype));
	ofd_prop_add(m, n, "model", model, sizeof(model));
	ofd_prop_add(m, n, "reg", reg, sizeof(reg));

	return n;
}


extern char default_bootargs[256];

static ofdn_t
ofd_chosen_props(void *m)
{
	ofdn_t n;
	ofdn_t p;
	static const char path[] = "/chosen";
	static const char console[] = " console=hvc0 nosmp";
	char b[257];
	uval sz = 0;

//		" root=/dev/hda "
//		" rootfstype=ramfs "
//		"init=/bin/sh "
//		"root=/dev/nfsroot "
//		"nfsroot=9.2.208.21:/,rsize=1024,wsize=1024 "
//		"ether=0,0,eth0 "
//nfsaddrs=<wst-IP>  :<srv-IP>  :<gw-IP>  :<netm-IP>    :<hostname>
//"nfsaddrs=9.2.208.161:9.2.208.21:9.2.208.2:255.255.248.0:freakazoid "
//"nfsaddrs=9.2.208.161:9.2.208.21:9.2.208.2:255.255.248.0:freakazoid:eth0 "

	n = ofd_node_find(m, path);
	if (n == 0) {
		n = ofd_node_add(m, OFD_ROOT, path, sizeof (path));
		ofd_prop_add(m, n, "name",
				&path[1], sizeof (path) - 1);
	}

	sz = strlen(default_bootargs);
	memcpy(b, default_bootargs, sz);

	assert(sz + sizeof(console) <= 256,
	       "boot args not big enough\n");

	memcpy(b + sz, console, sizeof (console));
	sz += sizeof (console);

	ofd_prop_add(m, n, "bootargs", b, sz);

	ofd_prop_add(m, n, "bootpath", NULL, 0);

	hputs("Remove /chosen/mmu, stub will replace\n");
	p = ofd_prop_find(m, n, "mmu");
	if (p > 0) {
		ofd_prop_remove(m, n, p);
	}

	return n;
}

static ofdn_t
ofd_memory_props(void *m, uval mem_size)
{
	ofdn_t n = 0;
	char fmt[] = "/memory@%lx";
	char name[] = "memory";
	uval32 v;
	uval start = 0;


	ofdn_t old;
	/* Remove all old memory props */
	do {
		old = ofd_node_find_by_prop(m, OFD_ROOT, "device_type",
					    name, sizeof(name));
		if (old <= 0) break;

		ofd_node_prune(m, old);
	} while (1);

	while (start < mem_size) {
		/* FIXME: these two properties need to be set during parition
		 * contruction */
		struct reg {
			uval64 addr;
			uval64 sz;
		};
		char path[64];
		uval l = snprintf(path, 64, fmt, start);
		n = ofd_node_add(m, OFD_ROOT, path, l+1);
		ofd_prop_add(m, n, "name", name, sizeof (name));

		v = 1;
		ofd_prop_add(m, n, "#address-cells", &v, sizeof (v));
		v = 0;
		ofd_prop_add(m, n, "#size-cells", &v, sizeof (v));

		ofd_prop_add(m, n, "device_type", name, sizeof (name));


			/* physical addresses usable without regard to OF */
		struct reg reg;
		reg.addr = start;
		reg.sz = mem_size - start;
		if (reg.sz > CHUNK_SIZE) {
			reg.sz = CHUNK_SIZE;
		}
		/* free list of physical addresses available after OF and
		 * client program have been accounted for */
		/* FIXME: obviously making this up */
		if (start == 0) {
			struct reg avail = {
				.addr = 0,
				.sz = CHUNK_SIZE - (1024 * 1024),
			};
			ofd_prop_add(m, n, "available", &avail,
				     sizeof (avail));
		}

		ofd_prop_add(m, n, "reg", &reg, sizeof (reg));

		start += reg.sz;
	}

	return n;
}


static ofdn_t
ofd_prune(void *m, const char *devspec)
{
	ofdn_t n;
	int rc = -1;
	while ((n = ofd_node_find(m, devspec)) > 0) {
		rc = ofd_node_prune(m, n);
	}

	return rc;
}

uval
ofd_devtree_init(uval mem, uval *space)
{
	uval sz;
	uval mapped;

	/* map the first page, there may be more */
	mapped = map_pages(mem, mem, PGSIZE);

	sz = ofd_size(mem);

	*space = ofd_space(mem);

	if (sz > mapped) {
		/* map the rest */
		map_pages(mem + mapped, mem + mapped, sz - mapped);
	}

#ifdef OFD_DEBUG
	ofd_walk((void *)mem, OFD_ROOT, ofd_dump_props, OFD_DUMP_VALUES);
#endif
	/* scan the tree and identify resources */
	ofd_pci_addr(mem);
	ofd_proc_dev_probe((void *)mem);

	if (ofd_platform_probe)
		ofd_platform_probe((void *)mem);

	*space -= 8;
	return mem;
}


uval
ofd_cleanup(uval ofd)
{
	uval space = ALIGN_UP(ofd_space(ofd), PGSIZE);
	hfree((void*)ofd, space);
	return 0;
}

uval
ofd_lpar_create(struct partition_status *ps, uval new, uval mem)
{
	void *m;
	uval sz;
	uval space;
	const ofdn_t n = OFD_ROOT;
	ofdn_t r;

	if (iohost_lpid == 0) {
		iohost_lpid = ps->lpid;
	}

	sz = ofd_size(mem);
	space = ofd_space(mem);

	if (new == 0) {
		new = (uval)halloc(ALIGN_UP(space, PGSIZE));
	}

	m = (void *)new;

	memcpy(m, (void *)mem, sz);
	memset((void *)(new + sz), 0, space - sz);

	hputs("Add /vdevice\n");
	ofd_vdevice(m, ps);

	hputs("Add /props\n");
	ofd_root_props(m, ps->lpid);

	hputs("Add /openprom props\n");
	ofd_openprom_props(m);

	hputs("Add /aliases props\n");
	ofd_aliases_props(m);

	hputs("Add /options props\n");
	ofd_options_props(m);

	/* yes, this is magic for now */
	/* MUST do it before CPUs */
	if (ps->lpid == iohost_lpid) {
		config_hba(m, ps->lpid);
	} else {
		hputs("Remove all IO busses, for all but the IOHost\n");
		ofd_prune(m, "pci");
		r = ofd_prop_find(m, n, "platform-open-pic");
		if (r > 0) {
			ofd_prop_remove(m, n, r);
		}
		ofd_prune(m, "FAKEOFF");
		ofd_prune(m, "ht");
		ofd_prune(m, "hostbridge");
	}

	hputs("Add /cpus props\n");
	ofd_cpus_props(m, ps);


	if (iohost_lpid != ps->lpid) {
		hputs("Add /interrupt-controller props\n");
		ofd_xics_props(m);
	} else {
		hputs("Add /platform-open-pic prop\n");
		ofd_openpic_owner_props(m, ps->lpid);
	}

	hputs("Add /chosen props\n");
	ofd_chosen_props(m);

	hputs("fix /memory@0 props\n");
	ofd_memory_props(m, ps->init_mem_size);

	hputs("Remove /rtas, client stub creates its own\n");
	ofd_prune(m, "rtas");


	if (ofd_platform_config) {
		ofd_platform_config(ps, m);
	}

	r = ofd_prop_add(m, n, "ibm,partition-no",
			 &ps->lpid, sizeof(ps->lpid));
	assert (r > 0, "oops");
	r = ofd_prop_add(m, n, "ibm,partition-name",
			 ps->name, strlen(ps->name)+1);
	assert (r > 0, "oops");

	return new;
}

void
ofd_bootargs(uval ofd, char *buf, uval sz)
{
	void *m = (void *)ofd;
	ofdn_t n;
	static const char path[] = "/chosen";

	if (buf[0] != '\0') {
		return;
	}

	n = ofd_node_find(m, path);
	if (n == 0) {
		return;
	}
	ofd_getprop(m, n, "bootargs", buf, sz);

}

