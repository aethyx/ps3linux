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
#include <lib.h>
#include <hypervisor.h>
#include <cpu.h>
#include <openfirmware.h>
#include <of_devtree.h>
#include <boot.h>
#include <rtas.h>
#include <asm.h>
#include <hv_regs.h>
#include <stack.h>
#include <thread_control_area.h>
#include <sim.h>
#include <asm.h>
#include <thinwire.h>

struct boot_of {
	uval bo_r3;
	uval bo_r4;
	uval bo_r5;
	uval bo_r6;
	uval bo_r7;
	uval bo_hrmor;
	uval bo_msr;
	uval bo_memblks;
	struct memblock *bo_mem;
};

static struct boot_of bof;
static ofdn_t boot_cpu;

#ifdef HV_USES_RTAS
static sval32 rtas_halt = -1;
static sval32 rtas_reboot = -1;
#endif

static inline void
debug_printf(const char *fmt, ...)
{
#if DEBUG_INITIALISATION
	va_list ap;

	va_start(ap, fmt);
	vhprintf(fmt, ap);
	va_end(ap);
#else
	(void)fmt;
#endif
}

struct isa_reg_property {
	uval32 space;
	uval32 address;
	uval32 size;
};

void *
boot_init(uval r3, uval r4, uval r5, uval r6, uval r7, uval boot_msr)
{
	/* save all parameters */
	bof.bo_r3 = r3;
	bof.bo_r4 = r4;
	bof.bo_r5 = r5;
	bof.bo_r6 = r6;
	bof.bo_r7 = r7;
	bof.bo_msr = boot_msr;

	/* Initialize the OF client layer */
	of_init(r5, boot_msr);

	bof.bo_hrmor = get_hrmor();
	return &bof;
}

struct io_chan *
boot_console(void *b __attribute__ ((unused)))
{
	return ofcon_init();
}

static struct memblock _hwmem[16];
uval
boot_memory(void *bp, uval img_start, uval img_end)
{
	struct boot_of *b = (struct boot_of *)bp;
	struct memblock *hwmem;
	phandle pkg;
	phandle root;
	uval32 addr_cells;
	uval32 size_cells;
	sval32 ret;
	int x = 0;
	uval sum = 0;

	hprintf("boot args: 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx\n"
		"hrmor: 0x%lx\n",
		bof.bo_r3,
		bof.bo_r4, bof.bo_r5, bof.bo_r6, bof.bo_r7, bof.bo_hrmor);

	assert(b->bo_r5 > img_end || b->bo_r5 < img_start,
	       "Hmm.. OF[0x%lx] seems to have stepped on our image "
	       "that ranges: 0x%lx .. 0x%lx.\n", b->bo_r5, img_start, img_end);

	// someday we will have an allocator
	hwmem = &_hwmem[0];

	root = of_finddevice("/");
	pkg = of_getchild(root);
	assert(pkg != OF_FAILURE, "root has no children");

	of_getprop(root, "#address-cells", &addr_cells, sizeof (addr_cells));
	of_getprop(root, "#size-cells", &size_cells, sizeof (size_cells));

	do {
		char type[32];
		uval32 reg[16];
		uval32 *y;
		const char memory[] = "memory";

		type[0] = 0;
		of_getprop(pkg, "device_type", type, sizeof (type));
		if (strncmp(type, memory, sizeof (memory)) == 0) {
			ret = of_getprop(pkg, "reg", reg, sizeof (reg));
			assert(ret != OF_FAILURE,
			       "no reg property for memory\n");

			y = &reg[0];

			if (addr_cells == 1) {
				hwmem[x].addr = *y;
				++y;
			} else if (addr_cells == 2) {
				hwmem[x].addr = (((uval64)y[0]) << 32) |
					(uval64)y[1];
				y += 2;
			}

			hwmem[x].addr += b->bo_hrmor;

			if (size_cells == 1) {
				hwmem[x].size = *y;
				++y;
			} else if (size_cells == 2) {
				hwmem[x].size = (((uval64)y[0]) << 32) |
					(uval64)y[1];
				y += 2;
			}
			/*
			 * FIXME: for some reason Stafford's rom
			 * reserves this area on HW, but we will blow it
			 * away by the time we use it
			 */
			if (hwmem[x].size == 0x0fff0000ULL) {
				hprintf("memory 0x%llx[0x%llx]\n",
					hwmem[x].addr, hwmem[x].size);
				hputs("WHAT is Stafford SMOKIN!!\nFIXING\n");
				hwmem[x].size = 0x10000000ULL;
			}
			hprintf("memory 0x%llx[0x%llx]\n",
				hwmem[x].addr, hwmem[x].size);
			sum += hwmem[x].size;
			++x;
		}
		pkg = of_getpeer(pkg);
	} while (pkg != OF_FAILURE && pkg != 0);

	b->bo_memblks = x;
	b->bo_mem = hwmem;

	return sum;
}

struct memblock *
boot_memblock(uval i)
{
	if (i >= bof.bo_memblks) {
		return NULL;
	}
	return &bof.bo_mem[i];
}

uval
boot_runtime_init(void *bp, uval eomem)
{
#ifdef HV_USES_RTAS
	struct boot_of *b = (struct boot_of *)bp;
	phandle rtas;
	static const char halt[] = "power-off";
	static const char reboot[] = "system-reboot";

	eomem = rtas_instantiate(eomem, b->bo_msr);

	if (rtas_entry) {
		rtas = of_finddevice("/rtas");
		assert(rtas > 0, "no /rtas, even though its initied\n");
		/* is this failes then the token stays -1 */
		of_getprop(rtas, halt, &rtas_halt, sizeof (rtas_halt));
		of_getprop(rtas, reboot, &rtas_reboot, sizeof (rtas_reboot));
	}
#endif
	(void)bp;
	return eomem;
}

static phandle
find_serial(void)
{
	char ofout_path[64] = { 0, };
	char alias_path[64] = { 0, };
	phandle ofout = -1;
	phandle pkg;
	sval32 ret = -1;

#ifdef OF_OUTPUT_DEVICE
	hprintf("Trying default output device: " OF_OUTPUT_DEVICE "\n");
	ofout = of_finddevice(OF_OUTPUT_DEVICE);
	if (ofout != OF_FAILURE) {
		return ofout;
	}
#endif

	/* probe /options tree */
	pkg = of_finddevice("/options");
	if (pkg != OF_FAILURE) {
		ret = of_getprop(pkg, "output-device",
				 ofout_path, sizeof (ofout_path));
	}
	if (ret == -1) {
		static const char serial[] = "serial";

		strncpy(ofout_path, serial, sizeof (serial));
	}

	hprintf("/options/output-device: %d\n", ret);

	/*
	 * if the options are not a path (they do not start with '/')
	 * then they are aliases and we must look them up.  we look it
	 * up in aliases because it is possible that the OF does not
	 * support finddevice() of an alias.
	 */
	if (ofout_path[0] != '/') {
		ret = -1;
		pkg = of_finddevice("/aliases");
		memcpy(alias_path, ofout_path, sizeof (alias_path));
		memset(ofout_path, 0, sizeof (ofout_path));
		if (pkg != OF_FAILURE) {
			ret = of_getprop(pkg, alias_path,
					 ofout_path, sizeof (ofout_path));
		}
	}
	if (ret != -1) {
		ofout = of_finddevice(ofout_path);

		hprintf("/options/output-device: %s(%x)\n", ofout_path, ofout);
	} else {
		/*
		 * Our last chance is to figure out the package for
		 * the current console and hopefully discover it to be
		 * a serial device.
		 */
		ihandle iout;

		ret = -1;
		pkg = of_finddevice("/chosen");
		if (pkg != OF_FAILURE) {
			ret = of_getprop(pkg, "stdout", &iout, sizeof (iout));
		}
		if (ret != -1) {
			ofout_path[0] = 0;
			ret = of_instance_to_path(iout, ofout_path,
						  sizeof (ofout_path));
			hprintf("path: %d %s\n", ret, ofout_path);
			if (ofout_path[0]) {
				ofout = of_finddevice(ofout_path);
			}
		}
	}
	return ofout;
}

static struct io_chan *
probe_serial(phandle node, uval32 phys_size)
{
	char type[64];
	static const char serial[] = "serial";
	static const char cutthru[] = "cutthru";
	static const char macio_name[] = "mac-io";
	uval32 dev_addr = 0;
	uval32 io_addr = 0;
	uval32 clock = 0;
	uval baudrate = BAUDRATE;
	uval sz;
	struct io_chan *(*init_fn) (uval io_addr, uval32 clock,
				    uval32 baudrate);
	struct io_chan *c;

	type[0] = 0;
	of_getprop(node, "device_type", type, sizeof (type));

	if (0 == strncmp(type, cutthru, sizeof (cutthru))) {
		c = mambo_thinwire_init();
		return c;
	}
	if (strncmp(type, serial, sizeof (serial))) {
		hprintf("Chosen is not a serial port\n");
		return NULL;
	}
	//We expect two parents at least
	//Even on non-Apple HW
	phandle escc = of_getparent(node);
	phandle macio = of_getparent(escc);

	if (escc == OF_FAILURE) {
		hprintf("Failed to get serial device parent\n");
		return NULL;
	}
	if (macio == OF_FAILURE) {
		hprintf("Failed to get serial device grandparent\n");
		return NULL;
	}

	/* Test to see if we've got a Mac serial port,
	 * by checking for the presence of a macio bus. */
	type[0] = 0;
	of_getprop(macio, "name", type, sizeof (type));

	if (!strncmp(type, macio_name, sizeof (macio_name))) {
		struct of_pci_range32_s *r32;
		struct reg_property32 reg;
		static const char escc_name[] = "escc";

		assert(phys_size == 32, "expecting 32bit addressing\n");

		type[0] = 0;
		of_getprop(escc, "device_type", type, sizeof (type));

		if (strncmp(type, escc_name, sizeof (escc_name))) {
			hprintf("parent is not escc: %s\n", type);
			return NULL;
		}

		sz = of_getproplen(macio, "ranges");
		assert(sz > 0, "no macio ranges property\n");

		r32 = alloca(sz);

		of_getprop(macio, "ranges", r32, sz);

		debug_printf("%x:%x:%x %x %x\n",
			     r32->opr_addr.opa_hi.word,
			     r32->opr_addr.opa_mid,
			     r32->opr_addr.opa_lo,
			     r32->opr_phys, r32->opr_size);

		of_getprop(node, "reg", &reg, sizeof (reg));

		dev_addr = reg.address;
		init_fn = zilog_init;
		io_addr = r32->opr_phys + dev_addr;
	} else {
		phandle isa;
		phandle pci;
		struct isa_reg_property reg;
		struct of_pci_range64_s *r64;

		assert(phys_size == 64, "expecting 64bit addressing\n");

		// We want parent and grand-parent,
		// which we already have
		isa = escc;
		pci = macio;

		of_getprop(node, "reg", &reg, sizeof (reg));

		of_getprop(node, "clock-frequency", &clock, sizeof (clock));

		sz = of_getproplen(macio, "ranges");
		assert(sz > 0, "no macio ranges property\n");

		r64 = alloca(sz);

		of_getprop(macio, "ranges", r64, sz);

		debug_printf("%x:%x:%x %x:%x %x:%x\n",
			     r64->opr_addr.opa_hi.word,
			     r64->opr_addr.opa_mid,
			     r64->opr_addr.opa_lo,
			     r64->opr_phys_hi,
			     r64->opr_phys_lo,
			     r64->opr_size_hi, r64->opr_size_lo);

		dev_addr = reg.address;
		init_fn = uartNS16750_init;
		io_addr = ((((uval64)r64->opr_phys_hi) << 32) |
			   (r64->opr_phys_lo)) + dev_addr;
	}

	debug_printf("serialPortAddr %lx\n", io_addr);
	debug_printf("clock-frequency: %x\n", clock);
	debug_printf("baudrate: %d\n", baudrate);

	serial_init_fn = init_fn;

	c = (*init_fn) (io_addr, clock, baudrate);

	return c;
}

struct io_chan *
boot_runtime_console(void *bofp)
{
	struct io_chan *c;
	phandle root;
	phandle ofout;
	uval32 cells;
	uval32 x = 1;

	(void)bofp;

#ifdef USE_THINWIRE_IO
	if (onsim()) {
		return mambo_thinwire_init();
	}
#endif

	if (io_chan_be_init != 0) {
		return io_chan_be_init();
	}
	// Interrogate OF for basic info
	root = of_finddevice("/");

	of_getprop(root, "#size-cells", &x, sizeof (x));
	if (x == 1) {
		cells = 32;
	} else {
		cells = 64;
	}
	hprintf("encode_phys_size: %d\n", cells);

	ofout = find_serial();

	if (ofout != -1) {
		c = probe_serial(ofout, cells);
		assert(c, "console is not a serial port!");
	} else {
		/* try the rtas device */
		c = io_chan_rtas_init();
	}

	return c;
}


static uval
save_props(void *m, ofdn_t n, phandle pkg)
{
	sval ret;
	char name[128];
	sval32 result = 1;
	uval found_name = 0;
	uval found_device_type = 0;
	const char name_str[] = "name";
	const char devtype_str[] = "device_type";

	/* get first */
	ret = call_of("nextprop", 3, 1, &result, pkg, 0, name);
	assert(ret == OF_SUCCESS, "nextprop");

	while (result > 0) {
		sval32 actual;
		sval32 sz;
		uval64 obj[128];
/* #define GETPROPLEN_SUPPORTED */
#ifdef GETPROPLEN_SUPPORTED
		sz = of_getproplen(pkg, name);
		if (sz >= 0) {
			ret = OF_SUCCESS;
		} else {
			ret = OF_FAILURE;
		}
#else	/* GET_PROPLEN_SUPPORTED */
		sz = sizeof(obj);
		ret = OF_SUCCESS;
#endif
		if (ret == OF_SUCCESS) {
			ofdn_t pos;

			if (sz > 0) {
				assert((uval)sz <= sizeof(obj),
				       "obj array not big enough for 0x%x\n",
				       sz);
				ret = call_of("getprop", 4, 1, &actual,
					      pkg, name, obj, sz);
				assert(ret == OF_SUCCESS, "getprop");
				assert(actual <= sz, "obj too small");
			}
			if (strncmp(name, name_str, sizeof(name_str))==0) {
				found_name = 1;
			}
			if (strncmp(name, devtype_str, sizeof(devtype_str))==0) {
				found_device_type = 1;
			}
			pos = ofd_prop_add(m, n, name, obj, actual);
			assert(pos != 0, "prop_create");
		}
		ret = call_of("nextprop", 3, 1, &result, pkg, name, name);
		assert(ret == OF_SUCCESS, "nextprop");
	}

//	assert(found_device_type, "Missing device type!\n");
//	assert(found_name, "Missing device type!\n");
	return 1;
}



/* Addresses of devices may need to be calculated relative to their
 * parent nodes.  This is best done going bottom up.  As we build our
 * copy of the tree we will add properites "#h_address", "#h_size",
 * which will contain the finalized address of the device.
 */
static void
add_hype_props(void *m, ofdn_t n, uval arg)
{
	(void) arg;
	static sval parse_regs(uval addr_cells, uval size_cells,
			       uval32 *regs,
			       uval* addr, uval* size) {

		if (addr_cells >= 2) {
			*addr = regs[addr_cells - 1] + regs[addr_cells - 2];
			regs += 2;
		} else {
			*addr = regs[0];
			++regs;
		}
		if (size_cells >= 2) {
			*size = regs[size_cells - 1] + regs[size_cells - 2];
		} else {
			*size = regs[0];
		}
		return 0;
	}

	char buf[128] = {0,};
	ofd_node_to_path(m, n, buf, sizeof(buf));

	sval ret;
	ofdn_t parent = ofd_node_parent(m, n);
	if (!parent) return;

	char type[32];
	sval len = ofd_getprop(m, parent, "device_type", type, sizeof(type));
	if (len <= 0) return;

	if (!strcmp(type, "pci")) {
		uval32 naddr;
		uval32 nsize;
		ret = ofd_getprop(m, parent, "#address-cells",
				  &naddr, sizeof(naddr));
		if (ret <= 0) {
			hprintf("No #address-cells property in pci bus\n");
			return;
		}

		ret = ofd_getprop(m, parent, "#size-cells",
				  &nsize, sizeof(nsize));
		if (ret <= 0) {
			hprintf("No #size-cells property in pci bus\n");
			return;
		}

		uval32 regs[16]; //16 should be big enough
		ret = ofd_getprop(m, n, "assigned-addresses",
				  regs, sizeof(regs));

		if (ret<(sval)(naddr+nsize)) {
			hprintf("pci device without assigned-addresses "
				"property: %ld %d %d\n\t%s\n",
				ret, naddr, nsize,buf);
			return;
		}
		uval addr;
		uval size;
		parse_regs(naddr, nsize, regs, &addr, &size);

		ofd_prop_add(m, n, "#h_address", &addr, sizeof(addr));
		ofd_prop_add(m, n, "#h_size", &size, sizeof(size));
	} else if (!strcmp(type,"mac-io")) {
		uval paddr;
		uval psize;
		ret = ofd_getprop(m, parent, "#h_address",
				  &paddr, sizeof(paddr));
		assert(ret == sizeof(paddr), "parent has no address\n");

		ret = ofd_getprop(m, parent, "#h_size",
				  &psize, sizeof(psize));
		assert(ret == sizeof(psize), "parent has no address\n");

		uval32 naddr;
		uval32 nsize;
		ret = ofd_getprop(m, parent, "#address-cells",
				  &naddr, sizeof(naddr));
		assert(ret>0, "No #address-cells property in macio bus\n");
		ret = ofd_getprop(m, parent, "#size-cells",
				  &nsize, sizeof(nsize));
		assert(ret>0, "No #size-cells property in macio bus\n");


		uval32 regs[16]; //16 should be big enough
		ret = ofd_getprop(m, n, "reg", regs, sizeof(regs));

		assert(ret>=(sval)(naddr+nsize),
		       "assigned-addresses not present");
		uval addr;
		uval size;

		parse_regs(naddr, nsize, regs, &addr, &size);
		addr += paddr;

		assert(addr < paddr + psize,
		       "Device address out of parent range\n");
		ofd_prop_add(m, n, "#h_address", &addr, sizeof(addr));
		ofd_prop_add(m, n, "#h_size", &size, sizeof(size));

	}
}


static sval
ofd_path_get(phandle pkg, char *path, uval len)
{
	sval ret;
	sval32 sz;

	ret = call_of("package-to-path", 3, 1, &sz,
		      pkg, path, len);
	if (ret == (sval32)OF_FAILURE) {
		assert(0, "of_peer: failed\n");
		return 0;
	}
	/*
	 * We could copy only the the last path component, but I doubt
	 * it is worth it since we will always have to rebuild the
	 * full path.
	 */
	return sz;
}


static void
do_pkg(void *m, ofdn_t n, phandle p, char *path, uval psz)
{
	phandle pnext;
	ofdn_t nnext;
	sval r;
	sval32 sz;

retry:
	save_props(m, n, p);

	/* do children first */
	r = call_of("child", 1, 1, &pnext, p);
	assert(r == OF_SUCCESS, "OF child failed\n");

	if (pnext != 0) {
		sz = ofd_path_get(pnext, path, psz);
		assert(sz > 0, "bad path\n");

		nnext = ofd_node_child_create(m, n, path, sz);
		assert(nnext != 0, "out of mem\n");

		do_pkg(m, nnext, pnext, path, psz);
	}

	/* do peer */
	r = call_of("peer", 1, 1, &pnext, p);
	assert(r == OF_SUCCESS, "OF peer failed\n");

	if (pnext != 0) {
		sz = ofd_path_get(pnext, path, psz);

		nnext = ofd_node_peer_create(m, n, path, sz);
		assert(nnext > 0, "out of mem\n");

		n = nnext;
		p = pnext;
		goto retry;
	}
}

static sval
pkg_save(void *mem)
{
	phandle root;
	char path[256];
	sval r;

	/* get root */
	r = call_of("peer", 1, 1, &root, 0);
	assert(r == OF_SUCCESS, "OF peer for root failed\n");

	do_pkg(mem, OFD_ROOT, root, path, sizeof(path));

	r = (((ofdn_t *)mem)[1] + 1) * sizeof (uval64);
#ifdef DEBUG
	hprintf("%s: saved device tree in 0x%lx bytes\n", __func__, r);
#endif
	return r;
}

static uval
boot_fixup_of_refs(void *mem)
{
	static const char *fixup_props[] = {
		"interrupt-parent",
	};
	uval i;
	uval count = 0;

	for (i = 0; i < ARRAY_SIZE(fixup_props); i++) {
		ofdn_t c;
		const char *name = fixup_props[i];

		c = ofd_node_find_by_prop(mem, OFD_ROOT, name, NULL, 0);
		while (c > 0) {
			const char *path;
			phandle rp;
			sval32 ref;
			ofdn_t dp;
			sval32 rc;
			ofdn_t upd;
			char ofpath[256];

			path = ofd_node_path(mem, c);
			assert(path != NULL, "no path to found prop: %s\n",
			       name);

			rp = of_finddevice(path);
			assert(rp > 0,
			       "no real device for: %s\n", path);

			rc = of_getprop(rp, name, &ref, sizeof(ref));
			assert(rc > 0, "no prop: %s\n", name);

			rc = of_package_to_path(ref, ofpath, sizeof (ofpath));
			assert(rc > 0, "no package: %s\n", name);

			dp = ofd_node_find(mem, ofpath);
			assert(dp >= 0, "no node for: %s\n", ofpath);

			ref = dp;

			upd = ofd_prop_add(mem, c, name, &ref, sizeof(ref));
			assert(upd > 0, "update failed: %s\n", name);

#ifdef DEBUG
			hprintf("%s: %s/%s -> %s\n", __func__,
				path, name, ofpath);
#endif
			++count;
			c = ofd_node_find_next(mem, c);
		}
	}
	return count;
}

static uval
boot_fixup_chosen(void *mem)
{
	sval32 ch;
	ofdn_t dn;
	ofdn_t dc;
	uval32 val;
	sval32 rc;
	char ofpath[256];

	ch = of_finddevice("/chosen");
	assert(ch > 0, "/chosen not found\n");

	rc = of_getprop(ch, "cpu", &val, sizeof (val));
	if (rc > 0) {
		rc = of_instance_to_path(val, ofpath, sizeof (ofpath));
		if (rc > 0) {
			dn = ofd_node_find(mem, ofpath);
			assert(dn >= 0, "no node for: %s\n", ofpath);

			boot_cpu = dn;
			val = dn;

			dn = ofd_node_find(mem, "/chosen");
			assert(dn > 0, "no /chosen node\n");

			dc = ofd_prop_add(mem, dn, "cpu", &val, sizeof (val));
			assert(dc > 0, "could not fix /chosen/cpu\n");
			rc = 1;
		} else {
			hputs("*** can't find path to booting cpu, "
				"SMP is disabled\n");
			boot_cpu = -1;
		}
	}
	return rc;
}

sval
boot_devtree(uval of_mem, uval sz)
{
	void *mem = (void *)of_mem;
	struct ofd_mem_s *m;
	sval r;

	mem = ofd_create(mem, sz);
	m = (struct ofd_mem_s *)mem;

	r = pkg_save(mem);

	boot_fixup_of_refs(mem);
	boot_fixup_chosen(mem);

	ofd_walk(mem, OFD_ROOT, add_hype_props, 2);

	return r;
}

extern uval _hype_stack_start[];
static uval
boot_cpu_start(uval ofd, phandle nodeid, uval cpuno, uval cpuseq)
{
	struct boot_cpu_info ci;
	uval stack;
	sval ret;

	stack = stack_new();
	ci.bci_stack = stack;
	ci.bci_msr = mfmsr();
	ci.bci_ofd = ofd;
	ci.bci_cpuno = cpuno;
	ci.bci_cpuseq = cpuseq;
	ci.bci_spin = 0;

	/* only thing we must have is a back chain of 0 */
	*((uval *)stack) = 0;

	ret = call_of("start-cpu", 3, 0, NULL, nodeid, boot_cpu_leap, &ci);
	if (ret != OF_SUCCESS) {
		return 0;
	}
	return boot_smp_watch(&ci);
}

uval
boot_cpus(uval ofd)
{
	static const char cpu[] = "cpu";
	void *m = (void *)ofd;
	ofdn_t c1;
	ofdn_t cn;
	uval cpus = 0;

	c1 = ofd_node_find_by_prop(m, OFD_ROOT, "device_type",
				   cpu, sizeof (cpu));

	assert(c1 > 0, "No CPUs found\n");
	cn = c1;

	do {
		++cpus;
		cn = ofd_node_find_next(m, cn);
	} while (cn > 0);

	hprintf("%s: found 0x%lx cpus\n", __func__, cpus);

	/* FIXME: there is no real way to pre-enumerate all the PIR
	 * values, so for now we assume that there are twice as many
	 * PIR values as CPUs to accomodate for unique PIR values in
	 * threads */
	tca_table_init(cpus * 2);
	cca_table_init(cpus);

	cn = c1;
	cpus = 0;
	while (cn > 0 && cpus < MAX_CPU) {
		phandle cp;
		const char *path;
		uval32 reg;
		sval sz;

		sz = ofd_getprop(m, cn, "reg", &reg, sizeof (reg));
		assert(sz == sizeof (reg), "no reg value\n");

		if (boot_cpu == -1 || cn == boot_cpu) {
			hprintf("initing booting CPU core: 0x%x\n", reg);
			cpu_core_init(ofd, cpus, reg);
		} else {
			path = ofd_node_path(m, cn);
			assert(path != NULL, "no path for CPU\n");

			cp = of_finddevice(path);
			assert(cp > 0, "%s: not found\n", path);

			boot_cpu_start(ofd, cp, reg, cpus);
		}
		++cpus;
		cn = ofd_node_find_next(m, cn);
		if (boot_cpu == -1) {
			break;
		}
	}
	return cpus;
}

extern uval _controller_start[0] __attribute__ ((weak));
extern uval _controller_end[0] __attribute__ ((weak));
extern uval _start[];
sval
boot_controller(uval *start, uval *end)
{
#ifdef DEBUG
	hprintf("boot args: %lx %lx %lx %lx %lx\n",
		bof.bo_r3, bof.bo_r4, bof.bo_r5, bof.bo_r6, bof.bo_r7);
#endif
	if (bof.bo_r3 || bof.bo_r4) {
		uval entry = *(uval *)&_start;

		*start = bof.bo_r3 - entry + 0x100;
		*end = *start + bof.bo_r4;
	} else {
		*start = (uval)_controller_start;
		*end = (uval)_controller_end;
	}
	return *end - *start;
}

void
boot_fini(void *bof)
{
#ifdef notyet
	ofcon_close();
#endif
	(void)bof;
	return;
}

void
boot_shutdown(void)
{
#ifdef HV_USES_RTAS
	if (rtas_entry && rtas_halt != -1) {
		struct rtas_args_s *r;

		r = alloca(sizeof (*r) + sizeof (r->ra_args[0]) * 4);
		r->ra_token = rtas_halt;
		r->ra_nargs = 2;
		r->ra_nrets = 1;
		r->ra_args[0] = 0;
		r->ra_args[1] = 0;
		rtas_call(r);

		hprintf("%s: !!!returned from shutdown!!!\n", __func__);
	}
#endif
	assert(0, "dunno how.\n");
}

void
boot_reboot(void)
{
#ifdef HV_USES_RTAS
	if (rtas_entry && rtas_reboot != -1) {
		struct rtas_args_s *r;

		r = alloca(sizeof (*r) + sizeof (r->ra_args[0]) * 4);
		r->ra_token = rtas_reboot;
		r->ra_nargs = 2;
		r->ra_nrets = 1;
		r->ra_args[0] = 0;
		r->ra_args[1] = 0;
		rtas_call(r);

		hprintf("%s: !!!returned from shutdown!!!\n", __func__);
	}
#endif
	assert(0, "dunno how.\n");
}
