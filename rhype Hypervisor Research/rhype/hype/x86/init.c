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
/*
 * Initialize the hypervisor.
 *
 */

#include <config.h>
#include <lib.h>
#include <asm.h>
#include <lpar.h>
#include <thinwire.h>
#include <hcall.h>
#include <hype.h>
#include <pmm.h>
#include <idt.h>
#include <os.h>
#include <vm.h>
#include <mmu.h>
#include <pgcache.h>
#include <multiboot.h>
#include <thread_control_area.h>
#include <h_proto.h>
#include <sched.h>
#include <util.h>
#include <io.h>
#include <timer.h>
#include <debug.h>
#include <objalloc.h>
#include <boot.h>
#include <gdbstub.h>
#include <vio.h>
#include <vtty.h>
#include <llan.h>
#include <aipc.h>
#include <crq.h>
#include <vterm.h>

struct pg_alloc phys_pa;
lock_t hype_mutex;
struct thread_control_area tca[MAX_CPU * THREADS_PER_CPU];	/* TCA space */
struct hype_control_area hca;	/* HCA space */

uval module_count = 0;
struct module {
	uval32 base;
	uval32 size;
	uval32 mem_start;
	uval32 mem_size;
	char cmdline[2048];
} modules[MAX_OS];

struct os *ctrl_os = NULL;

/*
 * The HV usually works out of the page tables of the cos
 * however, during startup, before any other OS is created
 * a special page table is needed.
 */
uval *hv_pgd;

uval cpuid_features;
uval cpuid_processor;
uval max_phys_memory;

extern uval _start[];
extern uval _end[];
extern uval __bss_start[];
extern char build_time[];
extern unsigned char _hype_stack[];

extern void hype_init(struct multiboot_info *mb_info)
	__attribute__ ((noreturn));

/*
 * Our multiboot bootloader has loaded the controlling OS and any other
 * guest OS images somwhere in memory. This routine picks up the images
 * and stores them for later processing.
 */
static void
multiboot(volatile struct multiboot_info *mb_info)
{
	volatile struct mod_info *mods_base;
	int i;

	/* various sanity checks */
	assert((mb_info->flags & MBFLAGS_MODS), "No modules specified");

	assert(mb_info->mods_count > 0, "No controlling OS specified");

	assert(mb_info->mods_count <= MAX_OS, "Too many guest OS specified");

	/*
	 * This first module is the image for the primary controller,
	 * the second module is for the second partitions, and so on.
	 * Copy the parameter to a safe place.
	 */
	module_count = mb_info->mods_count;
	mods_base = (struct mod_info *)mb_info->mods_addr;
	for (i = 0; i < (int)mb_info->mods_count; i++) {
		modules[i].base = mods_base[i].mod_start;
		modules[i].size =
			mods_base[i].mod_end - mods_base[i].mod_start;
		strncpy(modules[i].cmdline, (char *)mods_base[i].string,
			sizeof (modules[i].cmdline) - 1);
	}
}

/*
 * Handcraft a partition
 */
static struct os *
handcraft(struct module *mod, uval iopl)
{
	struct os *newos;
	uval paddr;
	sval rval;

	/* create partition and assign chunks */
	if ((newos = os_create()) == NULL)
		goto error;

#ifdef SPLIT_MEMORY
	if (mod->mem_size < CHUNK_SIZE) {
		define_mem(newos, MEM_ADDR, mod->mem_start, mod->mem_size);
	} else {
		define_mem(newos, MEM_ADDR, mod->mem_start, CHUNK_SIZE);
		define_mem(newos, MEM_ADDR, mod->mem_start + CHUNK_SIZE,
			   mod->mem_size - CHUNK_SIZE);
	}
#else
	define_mem(newos, MEM_ADDR, mod->mem_start, mod->mem_size);
#endif

	/* copy initial contents into the partitions */
	struct logical_chunk_info *lci = laddr_to_lci(newos, 0);

	if (lci == NULL) {
		goto error;
	}

	paddr = lci_translate(lci, 0, PGSIZE);

	/* copy initial content into the partition */
	copy_out((void *)(paddr + OS_LINK_BASE),
		 (const void *)virtual(mod->base), mod->size);

	uval64 magic = *(uval64 *)(virtual(mod->base) + 0x10);
	uval pinfo = INVALID_LOGICAL_ADDRESS;

	if (magic == HYPE_PARTITION_INFO_MAGIC_NUMBER) {
		pinfo = *(uval *)(virtual(mod->base) + 0x18);
	}

	/* setup OS and any initial devices */
	arch_os_init(newos, pinfo);
	if (newos->cpu[0] == NULL)
		goto error;

	/* copy command line arguments into the partition */
	paddr = paddr + CHUNK_SIZE - sizeof (mod->cmdline);
	copy_out((void *)paddr, mod->cmdline, sizeof (mod->cmdline));

	/* setup partition I/O priviledge level */
	newos->iopl = iopl;

	/*
	 * This wants to be locked, but we can get away with it here,
	 * since are initializing
	 */
	rval = locked_set_sched_params(newos->cpu[0], 0, 0x1, 0);
	if (rval != H_Success)
		goto error;

	rval = h_start(newos->cpu[0]->thread, newos->po_lpid,
		       OS_LINK_BASE /* pc */ , 0, 0,
		       CHUNK_SIZE - sizeof (mod->cmdline),	/* esi */
		       0, 0, 0);
	if (rval != H_Success)
		goto error;

	newos->cpu[0]->thread[0].preempt = CT_PREEMPT_YIELD_SELF;

#ifdef DEBUG
	hprintf("Created partition 0x%x ...\n", newos->po_lpid);
#endif

	return newos;

/* *INDENT-OFF* */
	/* get rid of this goto */
error:
/* *INDENT-ON* */
	if (newos) {
		os_free(newos);
	}
	return NULL;
}

static void
cpu_init(void)
{
	cpu_core_init();
}


/*
 * Initialize the hypervisor.
 *
 * The boot loader passes on the mb_info structure, which among other
 * things contains the memory map as provided by BIOS. Refer to multiboot
 * spec for details.
 */
void
hype_init(struct multiboot_info *mb_info)
{
	uval eomem;
	uval i;
	uval kmemsz;
	uval sz;
	struct io_chan *bio;
	struct io_chan *rio;
	void *b;

	eomem = (uval)_end;
	sz = eomem - (uval)__bss_start;

	/* clear bss */
	memset(__bss_start, 0, sz);

	/* Initialize boot support layer */
	b = boot_init(mb_info);

	/* get handle to boot console */
	bio = boot_console(b);
	/* all ouput goes to this io_chan */
	hout_set(bio);

	hprintf("Research Hypervisor/x86 version 0.01 (%s)\n", build_time);
	hprintf("Copyright (c) 2005 "
		"International Business Machines Corporation.\n\n");

	/*
	 * Sanity check!
	 *
	 * We pack in so many OSes including a linux kernel we need to
	 * make sure that this hypervisor image's address space fits
	 * in a chunk
	 */
	if ((eomem - (uval)_start) > CHUNK_SIZE - HV_LINK_BASE) {
		hputs("\n\nSorry, the hypervisor is way too big!\n");
		assert(0, "eomem: 0x%lx, chunk_size: 0x%lx",
		       eomem, CHUNK_SIZE);
	}

	kmemsz = boot_memory(b, (uval)_start, eomem);
	eomem = boot_runtime_init(b, eomem);

	rio = boot_runtime_console(b);
	assert(rio != NULL, "can't figure out runtime console\n");

	hputs("\nSwitching to runtime console\n");
	hout_set(rio);
	hputs("Switch to runtime console complete\n");

#ifdef USE_THINWIRE_IO
	{
		struct io_chan *tw;

		configThinWire(rio);
		tw = getThinWireChannel(CONSOLE_CHANNEL);
		hout_set(tw);
	}
#endif

#ifdef USE_GDB_STUB
	gdb_stub_init();
#endif

	/* parse bootstrap loader arguments */
	multiboot(mb_info);

	lock_init(&hype_mutex);

	/* initialize the hca */
	hca.eye_catcher = 0x48434129;
	hca.hcall_vector = NULL;
	hca.hcall_vector_len = 0;
	hca.hcall_6000_vector = NULL;
	hca.hcall_6000_vector_len = 0;

	for (i = 0; i < MAX_CPU; ++i) {
		int j;
		int tca_grp_index = i * THREADS_PER_CPU;
		struct thread_control_area *primary_tca = &tca[tca_grp_index];

		for (j = 0; j < THREADS_PER_CPU; ++j) {
			tca[tca_grp_index + j].eye_catcher = 0x54434120;
			tca[tca_grp_index + j].thread_index = j;
			tca[tca_grp_index + j].primary_tca = primary_tca;
			tca[tca_grp_index + j].hypeStack =
				(uval)(_hype_stack + (HV_STACK_SIZE * i)
				       + HV_STACK_SIZE - MIN_FRAME_SZ);
			tca[tca_grp_index + j].active_thread = NULL;
			tca[tca_grp_index + j].cached_hca = &hca;
		}
	}

	/*
	 * Save cpuid results in global variables.
	 */
	/* *INDENT-OFF* */
	__asm__ __volatile__ (
		"cpuid"
		: "=a" (cpuid_processor), "=d" (cpuid_features)
		: "a" (1)
		: "ebx", "ecx"
	);
	/* *INDENT-ON* */

	hprintf("\nCPUID: processor 0x%lx, features 0x%lx\n",
		cpuid_processor, cpuid_features);

	/* make sure the features we rely on exist */
	assert(cpuid_features & CPUID_FEATURES_PSE, "No page size extensions");
	assert(cpuid_features & CPUID_FEATURES_CX8, "No CMPXCHG8B support");

	/*
	 * Initialize the internal page allocator. Ideally, this is
	 * the space between the end of the hypervisor up to the end
	 * of a chunk. However, grub has loaded modules in this space
	 * which we have to be careful not to overwrite. We do this
	 * by marking the module pages as used and unmark them once
	 * we started the partitions.
	 * The only trick is, where does the page allocator store its
	 * freemap. Fortunately, grub leaves a enough room between the
	 * end of the kernel and the first module.
	 */
	uval top = eomem;
	for (i = 0; i < module_count; i++) {
		if (virtual(modules[i].base + modules[i].size) > eomem) {
			top = virtual(modules[i].base + modules[i].size);
		}
	}
	top = ALIGN_UP(top, PGSIZE);
	assert(CHUNK_SIZE - chunk_offset_addr(top) > PGSIZE,
	       "Not enough space for allocator init\n");
	pgalloc_init(&phys_pa, top, ALIGN_DOWN(top, CHUNK_SIZE),
		     CHUNK_SIZE - chunk_offset_addr(eomem), LOG_PGSIZE);


	uval start = ALIGN_UP(eomem, PGSIZE);
	free_pages(&phys_pa, start, top - start);
	hprintf("Page allocator space: 0x%lx - 0x%lx, 0x%lx - 0x%lx\n",
		start, top - 1, top, ALIGN_UP(top, CHUNK_SIZE) - 1);
	for (i = 0; i < module_count; i++) {
		if (virtual(modules[i].base) >= start &&
		    virtual(modules[i].base + modules[i].size) < top) {
			uval a = virtual(modules[i].base);
			uval b = a + modules[i].size;
			a = ALIGN_DOWN(a, PGSIZE);
			b = ALIGN_UP(b, PGSIZE);
			set_pages_used(&phys_pa, a, b - a);
			hprintf("Page allocator marked used: 0x%lx - 0x%lx\n",
				a,b - 1);
		}
	}


	init_allocator(&phys_pa);
	os_init();

	rcu_init();

	/* setup up hardware */
	pic_init();
	timer_init();

	vio_init();
	llan_sys_init();
	aipc_sys_init();
	crq_sys_init();
	vt_sys_init();
	
	
	cpu_init();
	/*
	 * Create and switch to initial HV page table these pages are not
	 * freed, all other OSs simply use these entries to map the HV.
	 *
	 * On entry, we are still using the page directory established in
	 * start.S. That page directory:
	 *
	 *      - is not reserved in any way
	 *      - identity maps us at 0 virtual (where the initial OS goes)
	 *
	 * That's bad, so we allocate and use a new page directory here.
	 */
	hv_pgd = (uval *)get_zeroed_page(&phys_pa);
	for (i = 0; i < HV_VSIZE; i += 1024 * PGSIZE) {
		uval pdi = ((i + HV_VBASE) & PDE_MASK) >> LOG_PDSIZE;

		hv_pgd[pdi] = (get_zeroed_page(&phys_pa) - HV_VBASE)
			| PTE_RW | PTE_P;
	}
	hv_map(HV_VBASE, 0x0, HV_VSIZE, PTE_RW, 0);

#ifdef USE_VGA_CONSOLE
	/* reserve a page for vga frame buffer and map it */
	uval base;

	base = hv_map(0, 0xb8000, PGSIZE, PTE_RW, 0);
	set_vga_base(base);
#endif

	uval mem_start = ALIGN_UP(physical(eomem), CHUNK_SIZE);
	uval mem_end = kmemsz * 1024;
	uval num_chunks = (mem_end - mem_start + HV_LINK_BASE) / CHUNK_SIZE;

	max_phys_memory = mem_end + HV_LINK_BASE;

	/* switch to hypervisor page directory */
	set_cr3(physical((uval)hv_pgd));

	/* initialize inter-partition copy address space area */
	imemcpy_init(eomem);

	assert(num_chunks >= module_count,
	       "Not enough memory available for all partitions");

	/*
	 * Assign chunks to partitions.
	 * Our initial assignment is one chunk per partition and
	 * any remaining chunks are assigned to the first partition.
	 */
	modules[0].mem_size = (1 + num_chunks - module_count) * CHUNK_SIZE;
	modules[0].mem_start = mem_start;
	mem_start += modules[0].mem_size;
	for (i = 1; i < module_count; i++) {
		modules[i].mem_start = mem_start;
		modules[i].mem_size = CHUNK_SIZE;
		mem_start += modules[i].mem_size;
	}

#ifdef DEBUG
	for (i = 0; i < module_count; i++) {
		hprintf("Partition %ld: 0x%x..0x%x (%d MB)\n", i,
			modules[i].mem_start,
			modules[i].mem_start + modules[i].mem_size,
			modules[i].mem_size >> 20);
	}
#endif

	/*
	 * Handcraft the specified partitions.
	 *
	 * The first partition (controller) is special and
	 * is given I/O privileges, others are not.
	 */
#ifdef NO_IO_PRIVILEGES		/* for debugging */
	ctrl_os = handcraft(&modules[0], EFLAGS_IOPL_0);
#else
	ctrl_os = handcraft(&modules[0], EFLAGS_IOPL_2);
#endif
	assert(ctrl_os != NULL, "Cannot create controller OS");

	free_pages(&phys_pa, virtual(modules[0].base), modules[0].size);

	pic_set_owner(&ctrl_os->cpu[0]->thread[0]);

	for (i = 1; i < module_count; i++) {
		struct os *rc;

		rc = handcraft(&modules[i], EFLAGS_IOPL_0);
		assert(rc != NULL, "Failed to create partition %ld\n", i);

		free_pages(&phys_pa, virtual(modules[i].base),
			   modules[i].size);
	}

	hprintf("About to resume into controller ...\n");
	arch_preempt_os(NULL, &ctrl_os->cpu[0]->thread[0]);
	resume_OS(&ctrl_os->cpu[0]->thread[0]);

	/* NOTREACHED */

	assert(0, "should never get here\n");
}
