/*
 * Copyright (C) 2005 Michal Ostrowski <mostrows@watson.ibm.com>, IBM Corp.
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

#include <hype.h>
#include <hype_util.h>
#include <util.h>
#include <partition.h>
#include <hypervisor.h>
#ifdef __i386__
#include <x86/os_args.h>
#endif

#include <limits.h>

#if 0
extern int execvp(const char *file, const char *const argv[]);

#define execvp glibc_execvp
#undef execvp
#endif
#include <unistd.h>

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <sys/mman.h>
#include <netinet/if_ether.h>

#include <sys/types.h>
#include <sys/wait.h>

int verbose = 0;

#define msg(fmt...) if (verbose) printf(fmt);

int hcall_fd;
oh_hcall_args hargs;

static uval crq;
static int vtys;
static int wait_for_key;
static uval console_ua = 0;

struct laddr_range {
	uval lr_base;
	uval lr_size;
};

struct laddr_range lranges[16] = { {0, 0}, };
int num_ranges = 0;

static void
usage()
{
	printf("oh_run_lpar [-n|--name <lpar>] [-m|--mem <addrspec>]\n"
	       "\t    [-h|--help] [-v|--verbose]\n"
	       "\t-n <lpar>     : Run the specified LPAR\n"
	       "\t-m <addrspec> : Add the specified memory to the LPAR\n"
	       "\t		  addrspec may be in the following forms,\n"
	       "\t		  all specify base:size:\n"
	       "\t		    0x8000000:0x4000000\n"
	       "\t		    0x80MB:64MB\n"
	       "\t		    2GB:1GB\n"
	       "\t-t [<n>]	: Make console a VTERM.\n"
	       "\t		  Optional <n> to create even more.\n"
	       "\t-c <crq>	: Add Client side CRQ (ie. vscsi).\n"
	       "\t-w		: wait for keypress before starting LPAR.\n"
	       "\t-v		: Be verbose\n"
	       "\t-h		: Print this message\n");
}

static void
bailout(const char *msg, ...)
{
	/* Eventually aborting will involve more than print and exit */
	va_list ap;

	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	exit(0);
}

static int
laddr_load(char *file, uval64 laddr, uval *base)
{
	uval64 chunk = ALIGN_DOWN(laddr, CHUNK_SIZE);
	uval64 offset = laddr - chunk;
	char *ptr = mmap(NULL, CHUNK_SIZE,
			 PROT_READ | PROT_WRITE, MAP_SHARED,
			 hcall_fd, chunk);

	ASSERT(ptr != MAP_FAILED, "mmap failure on /dev/hcall: %s\n",
	       strerror(errno));

	int fd = open(file, O_RDONLY);

	ASSERT(fd >= 0, "Can't open: %s\n", file);
	int ret = 1;
	int size = 0;

	do {
		ret = read(fd, ptr + offset, CHUNK_SIZE - offset);
		offset += ret;
		if (ret > 0)
			size += ret;

	} while (ret > 0);
	msg("Loaded to 0x%llx[0x%x] %s\n", laddr, size, file);
	
	*base = (uval)ptr;	
	return size;
}

static int
run_command(const char *const argv[])
{
	pid_t pid = fork();

	if (pid == 0) {
		int ret = execvp(argv[0], (char *const *)argv);

		printf("exec failure: %d %d %s\n", ret, errno,
		       strerror(errno));
		exit(-1);
	}
	int status;
	pid_t x = waitpid(pid, &status, 0);

	if (x != pid) {
		printf("pid mismatch: %d %d\n", x, pid);
		exit(-1);
	}
	return status;
}

static int
of_add_memory(uval64 laddr, uval64 size)
{
	char name[64];
	int ret;

	snprintf(name, 64, "/memory@0x%llx", laddr);

	ret = of_make_node(name);

	of_set_prop(name, "name", "memory", -1);
	of_set_prop(name, "device_type", "memory", -1);

	uval64 x = 2;

	of_set_prop(name, "#address-cells", &x, sizeof (x));
	of_set_prop(name, "#size-cells", &x, sizeof (x));

	uval64 reg[2] = { laddr, size };
	of_set_prop(name, "reg", &reg, sizeof (reg));
	return 0;
}

#ifdef __PPC__
static int
load_of(uval64 rmo_start, uval64 rmo_size)
{
	uval32 stub_addr = rmo_size - (1 << 20);
	char mem_node[64];
	uval64 prop[2];
	int ret;
	char *dum = NULL;

      retry:
	snprintf(mem_node, sizeof(mem_node), "/memory@0x%x", 0);
	prop[0] = 0;
	prop[1] = stub_addr;
	of_set_prop(mem_node, "available", prop, sizeof (prop));

	char oftree[512];
	char ofstub[512];
	char ofdimg[512];

	snprintf(oftree, sizeof(oftree), HYPE_ROOT "/%s/of_tree", oh_pname);
	snprintf(ofstub, sizeof(ofstub), HYPE_ROOT "/%s/of_image", oh_pname);
	snprintf(ofdimg, sizeof(ofdimg), HYPE_ROOT "/%s/of_tree.ofd", oh_pname);

	const char *const args[] = { "ofdfs", "pack", oftree, ofdimg, NULL };
	run_command(args);

	uval32 stub_size;
	struct stat buf;

	ret = stat(ofstub, &buf);
	ASSERT(ret == 0, "Stat failure %s\n", ofstub);
	stub_size = buf.st_size + 4 * PGSIZE;

	uval32 data_size;

	ret = stat(ofdimg, &buf);
	ASSERT(ret == 0, "Stat failure %s\n", ofdimg);
	data_size = buf.st_size;

	uval32 bottom = ALIGN_DOWN(rmo_size - data_size - stub_size, 1 << 20);

	if (stub_addr > bottom) {
		stub_addr = bottom;
		goto retry;
	}

	uval32 data_addr = ALIGN_UP(stub_addr + stub_size, PGSIZE);

	int fd = open(ofstub, O_RDWR);

	ASSERT(fd >= 0, "bad fd for %s\n", ofstub);

	off_t offset = lseek(fd, 0x10, SEEK_SET);

	ASSERT(offset != ((off_t) - 1), "seek failure on %s\n", ofstub);

	write(fd, &data_addr, sizeof (data_addr));

	close(fd);

	msg("Loading OF stub: 0x%x[0x%x]\n", stub_addr, stub_size);
	msg("Loading OF data: 0x%x[0x%x]\n", data_addr, data_size);
	laddr_load(ofstub, rmo_start + stub_addr, &dum);
	laddr_load(ofdimg, rmo_start + data_addr, &dum);

	set_file_printf("r5", "0x%lx", stub_addr);
	set_file_printf("r1", "0x%lx", stub_addr - PGSIZE);
	return 0;
}
#endif

static int
parse_args(int argc, char **argv)
{

	const struct option long_options[] = {
		{"name", 1, 0, 'n'},
		{"memory", 1, 0, 'm'},
		{"crq", 1, 0, 'c'},
		{"vterm", 2, 0, 't'},
		{"wait", 2, 0, 'w'},
		{"help", 0, 0, 'h'},
		{"verbose", 1, 0, 'v'}
	};

	while (1) {
		int ret = 0;
		int c = getopt_long(argc, argv, "n:m:c:t::hwv",
				    long_options, NULL);

		if (c == -1)
			return 0;

		switch (c) {
		case 'n':
			oh_pname = optarg;
			break;
		case 'c':
			crq = strtoul(optarg, NULL, 16);
			break;
		case 'm':{
				struct laddr_range *lr = &lranges[num_ranges];

				if (!parse_laddr
				    (optarg, &lr->lr_base, &lr->lr_size)) {
					++num_ranges;
				} else {
					ret = -1;
				}
				break;
			}
		case 'v':
			verbose = 1;
			break;

		case 'w':
			wait_for_key = 1;
			break;

		case 't':
			if (optarg == NULL) {
				vtys = 1;
			} else {
				vtys = strtol(optarg, NULL, 0);
				if (vtys == 0) {
					vtys = 1;
				}
			}
			break;

		default:
		case 'h':
			ret = -1;
		}

		if (ret < 0) {
			usage();
			exit(1);
		}
	}
	return 0;
}

static void
clean_vdevices()
{
	char scratch[128];
	snprintf(scratch, 128,
		 "rm -rf " HYPE_ROOT "/%s/of_tree/memory*", oh_pname);
	system(scratch);
	snprintf(scratch, 128,
		 "rm -rf " HYPE_ROOT "/%s/of_tree/vdevice/l-lan*", oh_pname);
	system(scratch);

	snprintf(scratch, 128,
		 "rm -rf " HYPE_ROOT "/%s/of_tree/vdevice/vty*", oh_pname);
	system(scratch);

	snprintf(scratch, 128,
		 "rm -rf " HYPE_ROOT "/%s/of_tree/vdevice/v-scsi*", oh_pname);
	system(scratch);
}

static int
add_memory(uval lpid, uval base, uval size)
{
	uval64 laddr;

	hargs.opcode = H_RESOURCE_TRANSFER;
	hargs.args[0] = MEM_ADDR;
	hargs.args[1] = 0;
	hargs.args[2] = base;
	hargs.args[3] = size;
	hargs.args[4] = lpid;
	int ret = hcall(&hargs);

	ASSERT(ret >= 0 && hargs.retval == 0,
	       "hcall failure: %d " UVAL_CHOOSE("0x%x", "0x%lx") "\n", ret,
	       hargs.retval);

	laddr = hargs.args[0];
	msg("add memory: %lx %lx -> %llx\n", base, size, laddr);

	of_add_memory(laddr, size);
	return 0;
}

static int
add_vty(uval32 liobn)
{
	const char fmt[] = "/vdevice/vty@%x";
	char vty_node[sizeof (fmt) + 8];

	snprintf(vty_node, sizeof (vty_node), "/vdevice/vty@%x", liobn);

	of_make_node(vty_node);
	of_set_prop(vty_node, "name", "vty", -1);
	of_set_prop(vty_node, "compatible", "hvterm1", -1);
	of_set_prop(vty_node, "device_type", "serial", -1);
	of_set_prop(vty_node, "reg", &liobn, sizeof (liobn));

	return 0;
}

static uval32
add_vterm(sval chan, uval lpid)
{
	int ret;

	hargs.opcode = H_VIO_CTL;
	hargs.args[0] = HVIO_ACQUIRE;
	hargs.args[1] = HVIO_VTERM;
	hargs.args[2] = 0;

	ret = hcall(&hargs);
	ASSERT(ret >= 0 && hargs.retval == 0,
	       "hcall failure: %d " UVAL_CHOOSE("0x%x", "0x%lx") "\n", ret,
	       hargs.retval);

	uval32 liobn = hargs.args[0];

	printf("vterm %lx liobn: %x\n", chan, liobn);
	hargs.opcode = H_RESOURCE_TRANSFER;
	hargs.args[0] = INTR_SRC;
	hargs.args[1] = liobn;
	hargs.args[2] = 0;
	hargs.args[3] = 0;
	hargs.args[4] = lpid;

	hcall(&hargs);
	ASSERT(ret >= 0 && hargs.retval == 0,
	       "hcall failure: %d " UVAL_CHOOSE("0x%x", "0x%lx") "\n", ret,
	       hargs.retval);

	/* FIXME on failure HVIO_RELEASE */

	if (chan < 0) {
		chan = liobn;
	}

	add_vty(chan);
	return liobn;
}

static int
add_vscsi(uval lpid, uval liobn, uval dma_sz)
{
	static const char fmt[] = "/vdevice/v-scsi@%x";
	char vscsi_node[sizeof (fmt) + 8 - 1];
	uval32 intr[2] = { /* source */ 0x0, /* +edge */ 0x0 };
	uval32 dma[] = {
		/* client */
		/* liobn */ 0x0,
		/* phys  */ 0x0, 0x0,
		/* size  */ 0x0, 0x0
	};
	uval32 val;
	int ret;

	hargs.opcode = H_RESOURCE_TRANSFER;
	hargs.args[0] = INTR_SRC;
	hargs.args[1] = liobn;
	hargs.args[2] = 0;
	hargs.args[3] = 0;
	hargs.args[4] = lpid;

	ret = hcall(&hargs);
	ASSERT(ret >= 0 && hargs.retval == 0,
	       "hcall vscsi transfer failure: %d " UVAL_CHOOSE("0x%x",
							       "0x%lx") "\n",
	       ret, hargs.retval);

	dma[0] = liobn;
	dma[4] = dma_sz;
	intr[0] = liobn;
	val = liobn;
	snprintf(vscsi_node, sizeof (vscsi_node), fmt, val);

	of_make_node(vscsi_node);
	of_set_prop(vscsi_node, "name", "v-scsi", -1);
	of_set_prop(vscsi_node, "device_type", "vscsi", -1);
	of_set_prop(vscsi_node, "compatible", "IBM,v-scsi", -1);
	of_set_prop(vscsi_node, "reg", &val, sizeof (val));
	of_set_prop(vscsi_node, "ibm,my-dma-window", dma, sizeof (dma));
	of_set_prop(vscsi_node, "interrupts", &intr, sizeof (intr));

	return 0;
}

static int
add_vdev(uval lpid, uval vdev)
{
	char p[512];
	int fd;
	int rc = -1;
	static const char vscsis[] = "v-scsi-host\n";

	snprintf(p, sizeof (p), "/sys/devices/vio/%lx/name", vdev);
	fd = open(p, O_RDONLY);
	if (fd != -1) {
		char name[64];

		rc = read(fd, name, sizeof (name));
		close(fd);
		if (rc > 0) {
			if (strncmp(name, vscsis, sizeof (vscsis)) == 0) {
				/* we know the lient crq is +1 */
				++vdev;
				printf("calling add_vscsi(0x%lx)\n", vdev);
				rc = add_vscsi(lpid, vdev, 0x00800000);
			}
		}
	}
	return rc;
}

static int
add_llan(uval lpid)
{
	int ret;

	hargs.opcode = H_VIO_CTL;
	hargs.args[0] = HVIO_ACQUIRE;
	hargs.args[1] = HVIO_LLAN;
	hargs.args[2] = ((8 << 12) >> 3) << 12;

	ret = hcall(&hargs);
	ASSERT(ret >= 0 && hargs.retval == 0,
	       "hcall failure: %d " UVAL_CHOOSE("0x%x", "0x%lx") "\n", ret,
	       hargs.retval);

	uval32 liobn = hargs.args[0];
	uval64 dma_sz = hargs.args[2];

	hargs.opcode = H_RESOURCE_TRANSFER;
	hargs.args[0] = INTR_SRC;
	hargs.args[1] = liobn;
	hargs.args[2] = 0;
	hargs.args[3] = 0;
	hargs.args[4] = lpid;

	ret = hcall(&hargs);
	ASSERT(ret >= 0 && hargs.retval == 0,
	       "hcall failure: %d " UVAL_CHOOSE("0x%x", "0x%lx") "\n", ret,
	       hargs.retval);

	/* FIXME on failure HVIO_RELEASE */

	uval32 dma[] = { liobn, 0, 0, dma_sz >> 32, dma_sz & 0xffffffff };
	uval8 mac[ETH_ALEN] = { 0x02, 0x00, };
	*(uval32 *)&mac[2] = liobn;

	char llan_node[64];

	snprintf(llan_node, sizeof(llan_node), "/vdevice/l-lan@%x", liobn);

	of_make_node(llan_node);
	of_set_prop(llan_node, "name", "l-lan", -1);
	of_set_prop(llan_node, "compatible", "IBM,l-lan", -1);
	of_set_prop(llan_node, "device_type", "network", -1);
	of_set_prop(llan_node, "reg", &liobn, sizeof (liobn));

	uval32 val = 2;

	of_set_prop(llan_node, "ibm,#dma-address-cells", &val, sizeof (val));
	of_set_prop(llan_node, "ibm,#dma-size-cells", &val, sizeof (val));

	val = 255;
	of_set_prop(llan_node, "ibm,mac-address-filters", &val, sizeof (val));

	val = 0;
	of_set_prop(llan_node, "ibm,vserver", &val, sizeof (val));
	of_set_prop(llan_node, "local-mac-address", mac, sizeof (mac));
	of_set_prop(llan_node, "mac-address", mac, sizeof (mac));
	of_set_prop(llan_node, "ibm,my-dma-window", dma, sizeof (dma));
	of_set_prop(llan_node, "interrupts", &liobn, sizeof (liobn));
	return 0;
}

#ifdef __PPC__
static int
add_htab(uval lpid, uval size)
{
	static uval32 ibm_pft_size[] = { 0x0, 0x0 };
	uval s = 1;
	uval lsize = 0;
	char cpu_node[64];
	uval cpu;
	int ret;

	while (s < size) {
		s <<= 1;
		++lsize;
	}

	/* 1 << i is now the smallest log2 >= size */
	lsize -= 6;		/* 1/64 of i */

	hargs.opcode = H_HTAB;
	hargs.args[0] = lpid;
	hargs.args[1] = lsize;

	ret = hcall(&hargs);
	ASSERT(ret >= 0, "hcall(HTAB, 0x%lx\n", lsize);

	ibm_pft_size[1] = lsize;

	cpu = 0;
	snprintf(cpu_node, sizeof (cpu_node), "cpus/cpu@%ld", cpu);
	of_set_prop(cpu_node, "ibm,pft-size",
		    ibm_pft_size, sizeof (ibm_pft_size));

	printf("ibm,pft-size: 0x%x, 0x%x\n", ibm_pft_size[0], ibm_pft_size[1]);

	return 0;
}
#endif

#ifdef __i386__
static int
fill_pinfo(struct partition_info *pinfo, uval lpid, uval rmo_size)
{
	int i;
	pinfo->lpid = lpid;
	for (i = 0; i < MAX_MEM_RANGES; i++) {
		pinfo->mem[i].size = INVALID_MEM_RANGE;
		i++;
	}
	pinfo->mem[0].size = rmo_size;
}

static int
inject_pinfo(struct partition_info *pinfo, uval length, 
             uval base, uval img_offset)
{
	int rc = 0;
	char *img_base = (char *)base;
	char *magic = &img_base[img_offset + 0x10];
	if (*(uval64 *)magic == HYPE_PARTITION_INFO_MAGIC_NUMBER) {
		uval offset = *((uval *)(magic + 8));
		memcpy(&img_base[offset], 
		       pinfo,
		       length);
		rc = 1;
	}
	return rc;
}

static int
add_cmdlineargs(uval base, uval offset, uval lpid)
{
	struct os_args os_args;
	const char stdparms[] = " pci=off ide=off";
	const char stdvdevs[] = " ibmvty=0x0,0x0,0x0,0xffffffff";
	char *img_base = (char *)base;
	(void)lpid;

	char oftree[512];
	char ofdimg[512];

	snprintf(oftree, sizeof(oftree), HYPE_ROOT "/%s/of_tree", oh_pname);
	snprintf(ofdimg, sizeof(ofdimg), HYPE_ROOT "/%s/vdevs", oh_pname);

	const char *const args[] = { "vdevs", "pack", oftree, ofdimg, NULL };

	run_command(args);

	get_file("of_tree/chosen/bootargs", 
	         os_args.commandlineargs, 
	         sizeof(os_args.commandlineargs));

	strncat(os_args.commandlineargs,
	        stdparms,
	        sizeof(os_args.commandlineargs));

	get_file("vdevs",
	         os_args.vdevargs,
	         sizeof(os_args.vdevargs));

	strncat(os_args.vdevargs,
	        stdvdevs,
	        sizeof(os_args.vdevargs));

	/* copy into partition memory */
	memcpy(&img_base[offset],
	       &os_args,
	       sizeof(os_args));

	if (verbose) {
		fprintf(stderr,"Cmd line: %s\n",
		        os_args.commandlineargs);
		fprintf(stderr,"vdevs   : %s\n",
		        os_args.vdevargs);
	}

	set_file_printf("r4", "0x%lx", offset);
}
#endif

int
main(int argc, char **argv)
{
	char scratch[128];

	hcall_fd = hcall_init();
	int ret = parse_args(argc, argv);

	if (ret < 0)
		return ret;
	uval total = 0;

	ASSERT(num_ranges > 0, "No memory ranges specified\n");

	ret = get_file("state", scratch, sizeof(scratch));
	if (ret <= 0 || strncmp(scratch, "READY", ret) != 0) {
		bailout("Partition not ready\n");
	}

	uval rmo_start = lranges[0].lr_base;
	uval rmo_size = lranges[0].lr_size;
	uval laddr = 0;
	uval img_base = 0;
	uval img_offset = 0;
	int count = 0;

	while (count < 255) {
		char image[256];
		char image_laddr[256];
		char data[64];
		struct stat sbuf;
		uval base;

		snprintf(image, sizeof(image), HYPE_ROOT "/%s/image%02x",
			 oh_pname, count);
		snprintf(image_laddr, sizeof(image_laddr), "image%02x_load", count);
		++count;

		ret = stat(image, &sbuf);
		if (ret < 0)
			continue;

		ret = get_file(image_laddr, data, sizeof(data));
		if (ret >= 0) {
			uval64 l = strtoull(data, NULL, 0);

			ASSERT(errno != ERANGE, "Corrupted data %s\n",
			       image_laddr);

			laddr = l;
		}

		int size = laddr_load(image, rmo_start + laddr, &base);

		if (0 == img_base) {
			img_base = base;
			img_offset = laddr;
		}

		laddr = ALIGN_UP(laddr + size, PGSIZE);
	}

	char pinfo_buf[64];
	uval64 pinfo;

	ret = get_file("pinfo", pinfo_buf, sizeof(pinfo_buf));

	if (ret <= 0 || (pinfo = strtoull(pinfo_buf, NULL, 0)) <= 0) {
		pinfo = (uval64)-1;
	}

	uval lpid;



	hargs.opcode = H_CREATE_PARTITION;
	hargs.args[0] = rmo_start;
	hargs.args[1] = rmo_size;
	hargs.args[2] = pinfo;

	ret = hcall(&hargs);
	ASSERT(ret >= 0 && hargs.retval == 0,
	       "hcall failure: %d " UVAL_CHOOSE("0x%x", "0x%lx") "\n", ret,
	       hargs.retval);

	lpid = hargs.args[0];

	set_file_printf("of_tree/ibm,partition-no", "0x%llx", lpid);
	set_file_printf("lpid", "0x%llx", lpid);
	set_file("state", "CREATED", -1);

	clean_vdevices();


	of_add_memory(0, rmo_size);
	total += rmo_size;

	int i = 1;

	while (i < num_ranges) {
		uval64 base = lranges[i].lr_base;
		uval64 size = lranges[i].lr_size;

		add_memory(lpid, base, size);
		total += size;
		++i;
	}

#ifdef __PPC__
	add_htab(lpid, total);
#endif

	if (get_file_numeric("res_console_srv", &console_ua) < 0) {
		console_ua = 0;
	} else {
		vtys = 1;
	}

	uval64 vty0 = 0;

	if (vtys == 0 && console_ua == 0) {
		add_vty(0);
	} else {
		if (vtys > 0) {
			/* add vty and force it to be vty@0 */
			vty0 = add_vterm(0, lpid);
			--vtys;
		}

		while (vtys > 0) {
			/* add vty and let it be the proper liobn */
			add_vterm(-1, lpid);
		}

	}

	if (crq > 0) {
		ret = add_vdev(lpid, crq);
		if (ret == -1) {
			fprintf(stderr, "vdev failed : 0x%lx\n", crq);
			return 1;
		}
	}

	if (console_ua) {
		printf("Registering console vterm: "
		       UVAL_CHOOSE("0x%lx","0x%llx")" -> 0x%lx:0x%llx\n",
		       console_ua, lpid, vty0);
		hargs.opcode = H_REGISTER_VTERM;
		hargs.args[0] = console_ua;
		hargs.args[1] = lpid;
		hargs.args[2] = vty0;
		ret = hcall(&hargs);
		ASSERT(ret >= 0 && hargs.retval == 0,
		       "hcall failure: %d " UVAL_CHOOSE("0x%x", "0x%lx") "\n",
		       ret, hargs.retval);
	}

	add_llan(lpid);

#ifdef __PPC__
	load_of(lranges[0].lr_base, lranges[0].lr_size);
#endif
#ifdef __i386__
	if (0 != img_base) {
		static struct partition_info part_info[2];
		fill_pinfo(&part_info[1], lpid, rmo_size);
		if (inject_pinfo(part_info, sizeof(part_info), 
		                img_base, img_offset)) {
			add_cmdlineargs(img_base, 
			                0x1000, /* offset in image for data */ 
			                lpid);
		}
	}
#endif

	hargs.opcode = H_SET_SCHED_PARAMS;
	hargs.args[0] = lpid;
	hargs.args[1] = 0;
	hargs.args[2] = 1;
	hargs.args[3] = 0;
	ret = hcall(&hargs);
	ASSERT(ret >= 0 && hargs.retval == 0,
	       "hcall failure: %d " UVAL_CHOOSE("0x%x", "0x%lx") "\n", ret,
	       hargs.retval);

	hargs.opcode = H_START;
	hargs.args[0] = lpid;

	char buf[64];

	ret = get_file("pc", buf, sizeof(buf));
	if (ret >= 0) {
		buf[ret] = 0;
		hargs.args[1] = strtoull(buf, NULL, 0);
	} else {
		hargs.args[1] = 0;
	}

	int x = 2;

	while (x < 8) {
		int y = 0;

		snprintf(buf, sizeof(buf), "r%d", x);
		y = get_file(buf, buf, sizeof(buf));
		if (y >= 0) {
			buf[y] = 0;
			hargs.args[x] = strtoull(buf, NULL, 0);
		} else {
			hargs.args[x] = 0;
		}
		++x;
	}

	if (wait_for_key) {
		printf("waiting for keypress:");
		(void)fgetc(stdin);
	}

	printf("Starting...\n");
	hcall(&hargs);
	ASSERT(ret >= 0 && hargs.retval == 0,
	       "hcall failure: %d " UVAL_CHOOSE("0x%x", "0x%lx") "\n", ret,
	       hargs.retval);

	set_file("state", "RUNNING", -1);
	return 0;
}
