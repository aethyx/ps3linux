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
#include <loadFile.h> /* I hate this file */
#include <partition.h>
#include <sysipc.h>
#include <xh.h>
#include <mmu.h>
#include <ofd.h>
#include <pgalloc.h>
#include <objalloc.h>
#include <util.h>
#include <bitmap.h>

#define THIS_CPU 0xffff
#define CHUNK_SIZE (1ULL << LOG_CHUNKSIZE)

#define IOHOST_NONE ~0UL
uval iohost_lpid = IOHOST_NONE;

#ifndef DEFAULT_LPAR_ARGS
#define DEFAULT_LPAR_ARGS  {0,}
#endif

/* these are magic and are defined by the linker */
extern char *image_names[];
extern uval image_cnt;

char default_bootargs[256] = DEFAULT_LPAR_ARGS;

extern uval curslots;
struct partition_status partitions[MAX_MANAGED_PARTITIONS];

/* This is for our memory */
struct pg_alloc pa;

/* This is for the system-wide logical-address space */
struct pg_alloc logical_pa;


static const char *default_input = CONTROLLER_INPUT;

static uval
get_input(char *buf)
{
	if (default_input[0]) {
		buf[0] = default_input[0];
		++default_input;
		return 1;
	}

	uval ret_vals[(16 / sizeof (uval)) + 1];
	uval rc;

	do {
		yield(1);
		rc = hcall_get_term_char(ret_vals, CONSOLE_CHANNEL);
		assert(rc == H_Success, "hcall_get_term_char: failed\n");
		vterm_service(partitions, MAX_MANAGED_PARTITIONS);
	} while (ret_vals[0] == 0);

	memcpy(buf, (char *)&ret_vals[1], ret_vals[0]);

	return ret_vals[0];
}

#if 0
static void
hexdump(unsigned char *data, int sz)
{
	unsigned char *d;
	int i;

	for (d = data; sz > 0; d += 16, sz -= 16) {
		int n = sz > 16 ? 16 : sz;
		hprintf("%04x: ", d - data);
		for (i = 0; i < n; i++)
			hprintf("%02x%c", d[i], i == 7 ? '-' : ' ');
		for (; i < 16; i++)
			hprintf("  %c", i == 7 ? '-' : ' ');
		hprintf("   ");
		for (i = 0; i < n; i++)
			hprintf("%c", d[i] >= ' ' && d[i] <= '~' ? d[i] : '.');
		hprintf("\n");
	}
}
#endif


static int
find_partition(uval lpid)
{
	int i;
	for (i = 0; i < MAX_MANAGED_PARTITIONS; ++i) {
		if (partitions[i].active == 1 &&
		    partitions[i].lpid == lpid) {
			return i;
		}
	}
	return -1;
}

static void
destroy_partition(int idx)
{
	uval rc = hcall_destroy_partition(NULL,
					  partitions[idx].lpid);
	if (rc == H_Success) {
		hprintf_nlk("\npartition 0x%lx destroyed\n",
			    partitions[idx].lpid);

		if (partitions[idx].lpid == iohost_lpid) {
			hprintf_nlk("\n\nWARNING: "
				"we just lost the IOHOST: 0x%lx: %s\n\n",
				partitions[idx].lpid,
				partitions[idx].name);
			iohost_lpid = IOHOST_NONE;
		}

		crq_release(partitions[idx].lpid);

		updatePartInfo(IPC_LPAR_DEAD,partitions[idx].lpid);
		partitions[idx].active = 0;

		free_pages(&logical_pa, partitions[idx].init_mem,
			   partitions[idx].init_mem_size);
		partitions[idx].init_mem = 0;
		partitions[idx].lpid = -1;
		partitions[idx].msgrcv = 0;
	} else {
		hprintf_nlk("partition destruction failed\n");
	}
}

static void
ask_debug_flags(void)
{
	char buf[17] = { 0, };
	int x;
	hprintf("Enter debug flags value:\n");
	get_input(buf);
	x = strtoul(buf, NULL, 0);

	hprintf("Debug value: %x\n", x);

	uval rets[5];
	hcall_debug(rets, H_SET_DEBUG_LEVEL, x, 0, 0, 0);

}
static void
ask_boot_args(void)
{
	char buf[257] = { 0, };
	int scanned = 0;
	int bytes = 0;
	hprintf("Enter boot args to be passed to next partition:\n");
	while (1) {
		if (bytes > (256 - 16)) {
			hprintf("Input string too long. Start over.\n");
			bytes = 0;
			scanned = 0;
		}
		int x = get_input(buf + bytes);
		if (x == 0) continue;

		bytes += x;
		while (scanned < bytes) {
			switch (buf[scanned]) {
			case '\n':
			case '\r':
				buf[scanned] = 0;
				++scanned;
				bytes = scanned;
				break;
			default:
				++scanned;
			}
		}

		if (scanned && buf[scanned-1] == 0) {
			break;
		}
	}

	if (scanned == 0) {
		return;
	}
	memcpy(default_bootargs, buf, scanned);
}


static void
ask_destroy_partition(void)
{
	char inbuf[17];
	uval sz;
	int done = 0;
	int i;

	while (!done) {
		uval val;
		hputs("Choose the partition to destroy\n");
		/* list active partitions */
		for (i = 0; i < MAX_MANAGED_PARTITIONS; ++i) {
			if (partitions[i].active == 1) {
				hprintf("  %02d: 0x%lx: %s\n", i,
					partitions[i].lpid,
					partitions[i].name);
			}
		}

		hputs("  e: exit this menu\n");
		hputs("Choice: ");

		sz = get_input(inbuf);
		inbuf[16] = '\0';
		if (sz == 0) continue;

		if (inbuf[0] == 'e') {
			/* fall into the destory menu */
			return;
		}

		val = strtoul(inbuf, NULL, 0);
		if (!((val < MAX_MANAGED_PARTITIONS) &&
		      (partitions[val].active == 1))) {
			hprintf("0x%lx is not a valid choice\n", val);
			continue;
		}
		destroy_partition(val);

	}
}

static void
controller_halt(void)
{
	uval i;

	for (i = 0; i < MAX_MANAGED_PARTITIONS; i++) {
		if (partitions[i].active) {
			destroy_partition(i);
		}
	}
	for (;;) {
		hcall_destroy_partition(NULL, H_SELF_LPID);
		hcall_yield(NULL, 0);
	}

}
static void
ask_schedule_partition(void)
{
	char inbuf[17];
	uval sz;
	int done = 0;
	int i;
	while (!done)
	{
		uval val;
		uval schedvals[3];
		sval rot;

		hprintf("Current schedules slots: 0x%08lx\n", curslots);
		/* list active partitions */
		for (i = 0; i < MAX_MANAGED_PARTITIONS; ++i) {
			if (partitions[i].active == 1) {
				hprintf("  %02d: 0x%lx: 0x%08lx: %s\n", i,
					partitions[i].lpid,
					partitions[i].slot,
					partitions[i].name);
			}
		}

		hputs("  e: exit this menu\n");
		hputs("Choice: ");

		sz = get_input(inbuf);
		inbuf[16] = '\0';
		if (sz == 0) continue;

		if (inbuf[0] == 'e') {
			return;
		}

		val = strtoul(inbuf, NULL, 0);
		if (!((val < MAX_MANAGED_PARTITIONS) &&
		      (partitions[val].active == 1)))
		{
			hprintf("0x%lx is not a valid choice\n", val);
			continue;
		}
		i = val;
		hprintf("new slot allocation for %s[0x%lx]: ",
			partitions[i].name, partitions[i].lpid);
		sz = get_input(inbuf);
		inbuf[16] = '\0';
		if (sz == 0) {
			continue;
		}
		val = strtoul(inbuf, NULL, 0);
		if (val == 0) {
			hputs("invalid slot allocation\n");
			continue;
		}
		hprintf("setting sched slot for 0x%lx to 0x%lx\n",
			partitions[i].lpid, val);
		rot = hcall_set_sched_params(schedvals, partitions[i].lpid,
					     THIS_CPU, val, 0);
		assert(rot >= 0, "set sched failed\n");
		hprintf("Scheduler controller: rot: "
			"%ld (0x%016lx, 0x%016lx 0x%016lx)\n",
			rot, schedvals[0], schedvals[1], schedvals[2]);
		partitions[i].slot = schedvals[2];
		curslots = schedvals[0];
	}
}

static void
ask(uval deflist, uval ofd)
{
	int i;
	char inbuf[17];
	uval sz;
	uval chunks = 1;
	uval lpalgn = LOG_CHUNKSIZE;

	for (i = 0; i < 4; i++) {
		uval def;
		def = deflist & (0xffUL << (8 * i));
		def >>= 8 * i;
		if (def == 0 || def >= image_cnt) {
			continue;
		}

		if (image_names[def - 1] != NULL) {
			uval mem = get_pages_aligned(&logical_pa,
						     CHUNK_SIZE,
						     LOG_CHUNKSIZE);
			assert(mem != PAGE_ALLOC_ERROR,
			       "no memory for partition\n");

			launch_image(mem, CHUNK_SIZE,
				     image_names[def - 1], ofd);

			yield(1);
		}
	}

	for (;;) {
		const char *iostr;
		uval iolpid = iohost_lpid;
		i = 0;
		hputs("Choose one of the following images\n");
		while (image_names[i] != NULL) {
			hprintf("  %02d: %s\n", i + 1, image_names[i]);
			++i;
		}

		switch (iohost_lpid) {
		case 0:
			iostr = "Next";
			break;
		case 1:
			iostr = "Unavailable";
			break;
		case IOHOST_NONE:
			iostr = "None";
			iolpid = 0;
			break;
		default:
			i = find_partition(iohost_lpid);
			iostr = partitions[i].name;
			break;
		}

		hprintf("  i: IOHost Selection: 0x%lx: %s\n", iolpid, iostr);
#ifdef USE_OPENFIRMWARE
		hputs("  o: Dump OF master device tree (if available)\n");
#endif
		if (rags_state != NULL) {
			hputs("  R: Resource Allocation Management: ");
			if (rags_state()) {
				hputs("ON\n");
			} else {
				hputs("OFF\n");
			}
		}

		hputs("  d: destroy a partition\n");
		hputs("  b: trigger breakpoint\n");
		hputs("  B: trigger HV Core breakpoint\n");
		hputs("  H: set HV debug verbosity level\n");
		hprintf("  A: set default partition boot arguments\n\t%s\n",
			default_bootargs);
		hputs("  s: schedule partitions\n");
		hputs("  h: halt machine\n");
		hputs("  y: yield forever\n");
		hprintf("  M: partition size (%ld x CHUNK_SIZE) (1..9)\n",
			chunks);
		hputs("Choice [1]: ");

		sz = get_input(inbuf);
		inbuf[16] = '\0';

		if (sz == 0) {
			continue;
		}
		hprintf("Got command: %s %ld\n",inbuf,sz);
		uval val;
		switch (inbuf[0]) {
		case 'y':
			hcall_yield(NULL, 0);
			break;
		case 'b':
			breakpoint();
			break;
		case 'H':
			ask_debug_flags();
			break;
		case 'A':
			ask_boot_args();
			break;
		case 'B':
			hcall_debug(NULL, H_BREAKPOINT, 0, 0, 0, 0);
			break;
		case 'h':
			controller_halt();
			break;
		case 'd':
			ask_destroy_partition();
			break;
		case 's':
			ask_schedule_partition();
			break;
		case 'M':{
			uval x = 1;
			while (inbuf[x]) {
				if (inbuf[x]>='1' && inbuf[x]<='9') {
					chunks = inbuf[x] - '0';
					break;
				}
				++x;
			}
			break;
		}
		case 'i':
			switch (iohost_lpid) {
			case IOHOST_NONE:
				iohost_lpid = 0;
				break;
			case 0:
				iohost_lpid = IOHOST_NONE;
				break;
			case 1:
				break;
			default:
				i = find_partition(iohost_lpid);

				assert (i >= 0, "bad iohost_lpid?\n");
				hprintf("IO Host already selected: "
					"0x%lx: %s\n",
					partitions[i].lpid,
					partitions[i].name);
				break;
			}
			break;
#ifdef USE_OPENFIRMWARE
		case 'o':
			if (ofd > 0) {
				ofd_walk((void *)ofd, OFD_ROOT,
					 ofd_dump_props, OFD_DUMP_ALL);
			} else {
				hputs("sorry no Of tree available\n");
			}
			break;
#endif
		case 'R':
			if (rags_ask) {
				rags_ask();
			}
			break;
		case '0' ... '9':
			val = strtoul(inbuf, NULL, 0) - 1;
			if (val < image_cnt) {
				uval mem;

				mem = get_pages_aligned(&logical_pa,
							chunks * CHUNK_SIZE,
							lpalgn);


				if (mem == PAGE_ALLOC_ERROR) {
					/* So what do we do here? */
					assert(0,
					       "no memory for partition\n");
				}

				if (mem != PAGE_ALLOC_ERROR) {
					launch_image(mem, chunks * CHUNK_SIZE,
						     image_names[val], ofd);
				}
			}
			break;
		default:
			hprintf("invalid entry: %s\n", inbuf);
			break;
		}
	}
}

void
updatePartInfo(uval action, int lpid)
{
	int i;
	for (i = 0; i < MAX_MANAGED_PARTITIONS; ++i) {
		if (partitions[i].msgrcv) {
			int ret = hcall_send_async(NULL, partitions[i].lpid,
						   action, lpid, 0, 0);
			if (ret != H_Success) {
				hprintf("Failed send: %d\n", ret);
			} else {
				/* directed yield */
				hcall_yield(NULL, lpid);
			}
		}
	}
}

#ifndef RELOADER
static void
sendPartInfo(int lpid)
{
	int i;
	for (i = 0; i < MAX_MANAGED_PARTITIONS; ++i) {
		if (partitions[i].lpid != (uval)-1) {
			int ret = hcall_send_async(NULL, lpid,
						   IPC_LPAR_RUNNING,
						   partitions[i].lpid,0,0);
			if (ret != H_Success) {
				hprintf("Failed send: %d\n", ret);
			} else {
				/* directed yield */
				hcall_yield(NULL, lpid);
			}
		}
	}
}

static void
msgHandler(struct async_msg_s *am)
{
	switch (am->am_data.am_mtype) {
		int i;

	case IPC_REGISTER_SELF:
		hprintf_nlk("\nReceived registration from 0x%lx\n",
			    am->am_source);
		for (i = 0; i < MAX_MANAGED_PARTITIONS; ++i) {
			if (partitions[i].lpid == am->am_source) {
				partitions[i].msgrcv = 1;
				sendPartInfo(am->am_source);
				break;
			}
		}
		break;

	case IPC_SUICIDE:
		hprintf_nlk("\nReceived suicide request from 0x%lx\n",
			    am->am_source);
		for (i = 0; i < MAX_MANAGED_PARTITIONS; ++i) {
			if (partitions[i].lpid == am->am_source) {
				destroy_partition(i);
			}
		}
		break;
	case IPC_RESOURCES:
		hprintf_nlk("\nReceived resource notification from 0x%lx\n",
			    am->am_source);
		hprintf_nlk("Resources: %lx 0x%lx[%lx]\n",
			    am->am_data.amt_data.amt_resources.ams_type,
			    am->am_data.amt_data.amt_resources.ams_laddr,
			    am->am_data.amt_data.amt_resources.ams_size);
		break;
	}
}
#endif

extern struct partition_info pinfo[2];
extern uval _end[];

uval
test_os(uval argc, uval argv[])
{
	uval ret[1];
	sval rc;
	uval r3 = argv[0];
	uval r4 = argv[1];
	uval ofd;
	const uval slot = 0x1;
	uval schedvals[3];
	sval rot;
	uval nchunks = 0;
	uval ofd_size;
	uval i = 0;

	argc = argc;

	rrupts_off();

        /* Setting up the IPC structures */
#ifndef RELOADER
	ret[0] = aipc_config(msgHandler);
	if (ret[0] != H_Success) {
		hprintf("Failed create: %ld\n", ret[0]);
	}
#endif
	if (r3 == ~0UL) {
		/* we../src/bin/mambo-sti -n have cloned ourselves */
		hputs("hello world.. this is slave\n");
		rc = hcall_get_lpid(ret);
		assert(rc == H_Success, "hcall_get_lpid() failed\n");
		hprintf("I am partition ID %ld\n", ret[0]);
		yield(0);
	}

	hputs("hello world.. this is controller\n");
	rc = hcall_get_lpid(ret);
	assert(rc == H_Success, "hcall_get_lpid() failed\n");

	ofd = ofd_devtree_init(r4, &ofd_size);
	ofd_bootargs(ofd, default_bootargs, sizeof (default_bootargs));

	pgalloc_init(&pa, (uval)_end,  0, pinfo[1].mem[0].size, LOG_PGSIZE);

	if (ofd) {
		set_pages_used(&pa, (uval)ofd, ofd_size);
	}

	init_allocator(&pa);

	rrupts_on();

	struct mem_range *mem = &pinfo[1].mem[0];
	uval max_addr = mem[0].size;

	while (i < MAX_MEM_RANGES &&
	       mem[i].size != INVALID_MEM_RANGE) {
		nchunks += mem[i].size / CHUNK_SIZE;
		max_addr = mem[i].addr + mem[i].size;
		++i;
	}

	pgalloc_init(&logical_pa, ~((uval)0), 0, max_addr, LOG_PGSIZE);
	set_pages_used(&logical_pa, 0, CHUNK_SIZE);

	uval curr = CHUNK_SIZE;
	i = 0;
	while (i < MAX_MEM_RANGES &&
	       mem[i].size != INVALID_MEM_RANGE) {
		if (curr < mem[i].addr) {
			set_pages_used(&logical_pa, curr,
				       mem[i].addr - curr);
			curr = mem[i].addr;
		}
		if (curr < mem[i].addr + mem[i].size) {
			free_pages(&logical_pa, curr,
				   mem[i].addr + mem[i].size - curr);
			curr = mem[i].addr + mem[i].size;
		}
		
		if (curr == mem[i].addr + mem[i].size) {
			++i;
		}
	}

	assert(nchunks >= 1, "not enough chunks to create an LPAR\n");

	hprintf("setting my sched slot to 0x%0lx\n", slot);
	rot = hcall_set_sched_params(schedvals, H_SELF_LPID,
				     THIS_CPU, slot, 0);
	assert(rot >= 0, "set sched failed\n");
	hprintf("Scheduler controller: rot: "
		"%ld (0x%016lx, 0x%016lx 0x%016lx)\n",
		rot, schedvals[0], schedvals[1], schedvals[2]);

	curslots = schedvals[0];

#ifdef RELOADER
	reload_image(image_names[0], ofd);
#endif

	for (i = 0; i < MAX_MANAGED_PARTITIONS; ++i) {
		curslots &= ~partitions[i].slot;
		partitions[i].active = 0;

		partitions[i].init_mem = 0;
		partitions[i].init_mem_size = 0;

		partitions[i].lpid = -1;
		partitions[i].vterm = 0;

		partitions[i].slot = 0;
		partitions[i].msgrcv = 0;
		partitions[i].name = NULL;
	}


	ask(r3, ofd);

	assert(0, "controller: Should never get here\n");

	return 0;
}
