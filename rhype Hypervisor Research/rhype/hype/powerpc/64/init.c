/*
 * Initialize the hypervisor.
 * hype_init() is the entry point.
 *
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
 *
 */

#include <config.h>
#include <lib.h>
#include <asm.h>
#include <debug.h>

#include <lpar.h>
#include <thinwire.h>
#include <htab.h>
#include <hype.h>
#include <mmu.h>
#include <pmm.h>
#include <thread_control_area.h>

#include <openfirmware.h>
#include <of_devtree.h>
#include <boot.h>
#include <hba.h>
#include <func_instrument.h>
#include <h_proto.h>

#include <sched.h>

#include <io_xlate.h>
#include <eic.h>

#include <pgalloc.h>
#include <objalloc.h>
#include <resource.h>
#include <vio.h>
#include <llan.h>
#include <aipc.h>
#include <crq.h>
#include <vterm.h>
#include <sim.h>
#include <bitmap.h>
#include <lpidtag.h>
#include <stack.h>

extern void openpic_probe(void *ofd);

extern void gdb_stub_init(void);

extern char hype_vec[];
extern char hype_vecend[];

lock_t hype_mutex;		/* Big Hypervisor Lock */
struct hype_control_area hca;	/* HCA space */

struct os *ctrl_os = NULL;
struct pg_alloc phys_pa;

/*
 * Initialize the hypervisor.  Return the os struct that this
 * CPU is assigned to.
 */

/* parameters from the bootloader:
 * lfa - the list of load images that have been put in memory
 * on_hardware - bool indicating we're running under hw or simulator
 */
extern char __bss_start[];
extern char _end[];
extern uval _start[3];

static void
hca_init(void)
{
	hca.eye_catcher = 0x48434129;
	hca.hcall_vector = hcall_vec;
	hca.hcall_vector_len = hcall_vec_len;
	hca.hcall_6000_vector = hcall_vec6000;
	hca.hcall_6000_vector_len = hcall_vec6000_len;
}

static void
tca_activate(struct thread_control_area *tca)
{
	tca->state = TCA_STATE_ACTIVE;
}

/*
 * All SMTs and CPUs start execution here.
 */
void
thread_init(struct thread_control_area *tca)
{
	DEBUG_MT(DBG_INIT, "%s: starting %d\n", __func__, tca->thread_index);

	/* setup my tca anchor and mark this thread as active */
	tca_activate(tca);
	rcu_init();
	cpu_idle();
	assert(0, "point of no return - should not get here");
}

/*
 * Hypervisor initialization
 * Software starts here
 */

extern char build_time[];
void
hype_init(uval r3, uval r4, uval r5, uval r6, uval r7, uval orig_msr)
{
	uval eomem;
	uval exc_vec_sz;
	uval sz;
	struct io_chan *bio;
	void *b;
	uval retval = H_Function;
	struct thread_control_area *tca;
	uval ofd;
	uval hv_ra;		/* base of HV relative to memory */
	uval ctrl_ra;		/* base of controller relative to memory */
	uval ctrl_ea;		/* base of controller relative to HV */
	uval ctrl_start;
	uval ctrl_end;
	struct io_chan *rio;
	struct memblock *block;
	uval pir;

	eomem = (uval)_end;
	sz = eomem - (uval)__bss_start;

	/* clear bss */
	memset(__bss_start, 0, sz);

	/* Initialize boot support layer */
	b = boot_init(r3, r4, r5, r6, r7, orig_msr);

	/* get handle to boot console */
	bio = boot_console(b);
	*(volatile uval *)8 = 0x5555;

	/* all ouput goes to this io_chan */
	hout_set(bio);
	hputs("switch to runtime console complete\n");
/*#define JS20_SOL_DELAY 1*/
#ifdef JS20_SOL_DELAY
	if (!onsim()) {
		uval q = 0;
		while (q < 50) {
			hprintf("waiting for SOL %ld\n", q++);
			int x = 1<<20;
			while (x--) eieio();
		}
	}
#endif

	hprintf("\nResearch Hypervisor/PPC version 0.1 (%s)\n", build_time);
	hprintf("Copyright (c) 2005 "
		"International Business Machines Corporation.\n\n");

	hv_ra = get_hrmor();
	if (hv_ra != 0) {
		hprintf("%s: EA = 0x0 is RA = 0x%lx\n", __func__, hv_ra);
	}

	/*
	 * sanity check!
	 *
	 * We pack in so many OSes including a linux kernel we need to
	 * make sure that this hypervisor image's address space fits
	 * in a chunk
	 */
	if (eomem > CHUNK_SIZE) {
		hputs("\n\nSorry, Hypervisor is way to big!\n"
		      "Try removing some test images\n\n");
		assert(0, "_end: %p, chunk_size: 0x%lx\n", _end, CHUNK_SIZE);
	}
	hprintf("hype at: %lx - %lx\n", _start[0], (uval)_end);

	sz = boot_memory(b, _start[0], eomem);
	if (sz < (CHUNK_SIZE * 2)) {
		hputs("\n\nAt least two chunks are necessary\n");
		assert(0, "memsz: 0x%lx chunksz: 0x%lx\n", sz, CHUNK_SIZE);
	}
	eomem = boot_runtime_init(b, eomem);

	rio = boot_runtime_console(b);
	assert(rio != NULL, "can't figure out runtime console\n");

	hputs("switching to runtime console\n");
	hout_set(rio);

#ifdef USE_THINWIRE_IO
	{
		struct io_chan *tw;

		configThinWire(rio);
		tw = getThinWireChannel(CONSOLE_CHANNEL);
		hout_set(tw);
	}
#endif

	exc_vec_sz = (uval)hype_vecend - (uval)hype_vec;
	/* Copy the exception vector down to location zero. */
	DEBUG_OUT(DBG_INIT, "master_init: copying exception vectors "
	      "from %p to 0x0 - 0x%lx\n", hype_vec, exc_vec_sz);

	{
		/* GCC 3.4 will complain on memcpy(0, ..) we need a
		 * better solution for this, but this seem to shut it
		 * up for now */
		void *zero = (void *)0;

		memcpy(zero, hype_vec, exc_vec_sz);
	}

#ifdef USE_GDB_STUB
	/* no use in starting gdb until the vectors are copied down */
	gdb_stub_init();
#endif

	hprintf_mt("%s: starting %d\n", __func__, get_tca_index());

	/* initialize the global lock */
	lock_init(&hype_mutex);

	/* initialize inter-partition copy address space area */
	eomem = imemcpy_init(eomem);

	ctrl_ea = ALIGN_UP(eomem, 1ULL << LOG_CHUNKSIZE);
	// HACK JX
	ctrl_ra = ctrl_ea + hv_ra;

	uval ctrl_rmo_size = 1ULL << LOG_CHUNKSIZE;

	/* use the rest of the hypervisor chunk for internal page allocation */
	pgalloc_init(&phys_pa, eomem, 0, ctrl_ea, LOG_PGSIZE);

	init_allocator(&phys_pa);

	os_init();
	rcu_init();
	vio_init();
	llan_sys_init();
	aipc_sys_init();
	crq_sys_init();
	vt_sys_init();
	hca_init();
	/* place of devtree here */
	sz = 48 * PGSIZE;
	ofd = ctrl_ea + ctrl_rmo_size;
	ofd = ALIGN_DOWN(ofd - sz, PGSIZE);

	boot_devtree(ofd, sz);
	boot_cpus(ofd);

	/* get a new stack for this processor */
	uval stack = stack_new();

	tca = tca_from_stack(stack);
	tca_init(tca, &hca, stack, 0);
	tca_activate(tca);
	pir = mfpir();
	hprintf("%s: registering PIR: 0x%lx TCA: %p\n", __func__, pir, tca);
	tca_table_enter(pir, tca);
	cca_table_enter(0, tca->cca);

	assert(tca->thread_index == 0, "only thread 0 is alowed here.\n");

	/* this is the address relative to the next lpar */
	r4 = ofd - ctrl_ea;

	sz = boot_controller(&ctrl_start, &ctrl_end);
	assert(sz > 0, "Controller image has size of zero\n");

	DEBUG_MT(DBG_INIT, "%s: COS load [start, end, size]: "
		 "[0x%lx, 0x%lx, 0x%lx]\n",
		 __func__, ctrl_start, ctrl_end, sz);

	/* when we come back we should be on ouw own stack and not
	 * boot services should not longer be necessary */
	boot_fini(b);

	io_xlate_init(ofd);

	/* Initialize the controlling partition:
	 * copy the controlling partition into the first chunk it
	 * owns. Assumes that the OS image will fit in one chunk.
	 */
	ctrl_os = os_create();

	assert(ctrl_os != NULL, "os_create failed for controlling OS\n");

	uval taken = ctrl_ra + ctrl_rmo_size;

	define_mem(ctrl_os, MEM_ADDR, ctrl_ra, ctrl_rmo_size);
	copy_out((void *)ctrl_ra, (void *)ctrl_start, sz);


	/* Identify memory that we can free between the exception vector
	   and _start, and also the pre-copy controller image */
	uval begin = 0x4000;
	uval finish= ALIGN_DOWN(_start[0], PGSIZE);

	hprintf("Free memory: 0x%lx[0x%lx]\n", begin, finish - begin);
	free_pages(&phys_pa, begin, finish - begin);

	begin = ALIGN_UP(ctrl_start, PGSIZE);
	finish = ALIGN_DOWN(ctrl_end, PGSIZE);
	finish = MIN(finish, ctrl_ra);

	hprintf("Free memory: 0x%lx[0x%lx]\n", begin, finish - begin);
	free_pages(&phys_pa, begin, finish - begin);



	uval j = 0;
	uval mem_sum = 0;

	while ((block = boot_memblock(j++))) {
		uval size = block->size;
		uval addr = block->addr;

		while (taken > addr) {
			addr += CHUNK_SIZE;
			size -= CHUNK_SIZE;
		}
		if (size > 0) {
			define_mem(ctrl_os, MEM_ADDR, addr, size);
		}
		mem_sum += size;
	}
	if (mem_sum > 0) {
		hprintf("%s: WARNING: no memory after controller\n", __func__);
	}

	uval pinfo = *(uval *)(ctrl_start + 0x10);

	if (pinfo == HYPE_PARTITION_INFO_MAGIC_NUMBER) {
		pinfo = *(uval *)(ctrl_start + 0x18);
	} else {
		pinfo = INVALID_LOGICAL_ADDRESS;
	}
	arch_os_init(ctrl_os, pinfo);

	/* create a known-size htab for controller */
	htab_alloc(ctrl_os, LOG_DEFAULT_HTAB_BYTES);

	/*
	 * This is an H_START hcall() initiated "under the covers".
	 * There have been a bunch of implicit H_RESOURCE_TRANSFER
	 * hcall()s which will eventually need to be accounted for.
	 */

	DEBUG_MT(DBG_INIT, "master_init: starting partition 0x%x\n",
		 ctrl_os->po_lpid);

	/*
	 * Controller parition will always start at 0x0 and make it's
	 * own TOC if it needs it
	 */
	assert(ctrl_os->cpu[0] != NULL, "No CPU associated with OS\n");

	/* give contgrolling OS all PCI busses */
	retval = hba_init(ofd);
	assert(retval == H_Success, "hba configuration failed\n");

	/* This wants to be locked, but we can get away with it here,
	 * since were initializing. */
	retval = locked_set_sched_params(ctrl_os->cpu[0], 0, 0x1, 0);
	assert(retval == H_Success, "Failed initial scheduling\n");

	retval = h_start(ctrl_os->cpu[0]->thread, ctrl_os->po_lpid,
			 /* pc */ 0, /* toc */ 0,
			 r3, r4, 0, r6, r7);

	assert(retval == H_Success, "Failed initial h_start\n");

#ifdef HAS_HDECR
#ifdef DEBUG
	hprintf("%s: setting HDEC\n", __func__);
#endif
	/* set the HDEC counter for the first time */
	mthdec(5 << 16);
#endif

#ifdef CHECK_HDEC
	{
		register uval32 h[6];
		asm __volatile__("mfspr %0, %6; mfspr %1, %6;"
				 "mfspr %2, %6; mfspr %3, %6;"
				 "mfspr %4, %6; mfspr %5, %6":"=&r"(h[0]),
				 "=&r"(h[1]), "=&r"(h[2]), "=&r"(h[3]),
				 "=&r"(h[4]), "=&r"(h[5])
				 :"i"(SPRN_HDEC));

		hprintf("hdec 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
			h[0], h[1], h[2], h[3], h[4], h[5]);
	}
#endif

	DEBUG_MT(DBG_INIT, "%s: h_scheduler: %p\n",
		 __func__, &hype_per_cpu[0].hpc_sched);

	tca->cca->active_cpu = ctrl_os->cpu[0];

	/*
	 * Fall into the idle loop. The COS is scheduled and will be
	 * picked up and dispatched.
	 */
	/* start the secondary thread */
	DEBUG_MT(DBG_INIT, "master_init: releasing slave threads (if any)\n");
	DEBUG_MT(DBG_INIT, "tca %p, thread_idx %d, partner %p, "
		 "cca %p, cpu %p\n",
		   tca, tca->thread_index, tca->partner_tca, tca->cca,
		   tca->cca ? tca->cca->active_cpu : 0);

	start_partner_thread();
	/* when we exit and return in HV we will be on a new stack,
	 * sure would be nice to free the old one up somehow */
	cpu_idle();
}
