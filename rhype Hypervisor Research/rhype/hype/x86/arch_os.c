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

#include <os.h>
#include <hype.h>
#include <mmu.h>
#include <pmm.h>
#include <vm.h>
#include <idt.h>
#include <llan.h>
#include <pgcache.h>
#include <debug.h>
#include <objalloc.h>
#include <cpu_thread.h>
#include <crq.h>
#include <lpidtag.h>
#include <stack.h>

uval
arch_os_init(struct os *newos, uval pinfo_addr)
{
	struct partition_info *pinfo;
	char buffer[256];

	if (pinfo_addr != INVALID_LOGICAL_ADDRESS) {
		pinfo_addr = logical_to_physical_address(newos, pinfo_addr,
							 sizeof (*pinfo));

		copy_in(buffer, (void *)pinfo_addr,
			sizeof (buffer) / sizeof (buffer[0]));

		pinfo = (struct partition_info *)buffer;
		newos->os_partition_info = (struct partition_info *)pinfo_addr;

#ifdef DEBUG
		hprintf("Partition info at %p\n", (void *)pinfo_addr);
#endif
		copy_in(&newos->cached_partition_info[0],
			(void *)pinfo_addr, sizeof (struct partition_info));

	} else {
#ifdef DEBUG
		hprintf("No pinfo structure found; using cached pinfo\n");
#endif
		memset(&newos->cached_partition_info[0], 0,
		       sizeof (struct partition_info));

		pinfo = newos->os_partition_info =
			&newos->cached_partition_info[0];

	}

	newos->cached_partition_info[1].lpid = newos->po_lpid;

	newos->iopl = EFLAGS_IOPL_0;

	lpidtag_acquire(newos);
	vtty_init(newos);

	return 0;
}

/*
 * Save/restore machine state that changes with OS
 */
uval
arch_preempt_os(struct cpu_thread *thread __attribute__ ((unused)),
		struct cpu_thread *next_thread)
{
	/*
	 * On entry into the hv, we need to identify the thread which was in
	 * execution, we do this by saving a pointer to the thread into the TSS
	 */
	next_thread->tss.threadp = (uval)next_thread;

#ifdef CTXSW_DEBUG
	hprintf("arch_preempt_os: Context switch ");
	if (thread != NULL)
		hprintf("from LPID 0x%x ", thread->cpu->os->po_lpid);
	hprintf("to LPID 0x%x", next_thread->cpu->os->po_lpid);
	hprintf("(%%eip 0x%lx)\n", next_thread->tss.eip);
#endif

	return 0;
}

/*
 * This routine is called from preemt.S:resum_OS() #ifdef
 * HV_EXIT_DEBUG to check if the processor will be stored to a sane
 * state
 */
void
arch_validate(struct cpu_thread *thread)
{
	assert((thread->tss.srs.regs.cs & 0x3) != 0x0,
	       "Returning to partition with DS CPL == 0\n");
	assert((thread->tss.srs.regs.cs & 0x3) >= 0x2,
	       "Returning to partition with CPL != 2\n");
	assert((thread->tss.srs.regs.ss & 0x3) != 0x0,
	       "Returning to partition with SS CPL == 0\n");
	assert((thread->tss.srs.regs.ss & 0x3) >= 0x2,
	       "Returning to partition with SS CPL != 2\n");
	assert((thread->tss.eflags & EFLAGS_IF),
	       "Returning to partition with EFLAGS.EFLAGS_IF = 0\n");
}

void
arch_init_thread(struct os *os, struct cpu_thread *thread, uval pc,
		 uval edx, uval ebx, uval esi, uval edi, uval esp, uval ebp)
{
	gdt_init(thread);
	idt_init(thread);

	thread->tss.gprs.regs.eax = 0x48563836;	/* magic: "HV86" */
	thread->tss.gprs.regs.ecx = 0;	/* version 0 */

	thread->tss.gprs.regs.edx = edx;
	thread->tss.gprs.regs.ebx = ebx;
	thread->tss.gprs.regs.esi = esi;
	thread->tss.gprs.regs.edi = edi;
	thread->tss.gprs.regs.esp = esp;
	thread->tss.gprs.regs.ebp = ebp;

	thread->tss.srs.regs.cs = __GUEST_CS;
	thread->tss.srs.regs.ds = __GUEST_DS;
	thread->tss.srs.regs.ss = __GUEST_DS;
	thread->tss.srs.regs.es = __GUEST_DS;
	thread->tss.srs.regs.fs = __GUEST_DS;
	thread->tss.srs.regs.gs = __GUEST_DS;

	/* SS/ESP1,2 should be filled in by the OS */
/* XXX there is already a hypervisor thread stack in the tca structure */
	thread->tss.esp0 = get_pages(&phys_pa, HV_STACK_SIZE)
		+ HV_STACK_SIZE;
	thread->tss.ss0 = __HV_DS;
	thread->tss.eip = pc;
	thread->tss.eflags = os->iopl | EFLAGS_IF;
	thread->tss.iomap_base_addr = 0x0;	/* IO map is not used */

	thread->reg_cr0 = CR0_PG | CR0_AM | CR0_WP | CR0_NE | CR0_MP
		| CR0_ET | CR0_PE;
	thread->reg_cr2 = 0;
	thread->reg_cr4 = CR4_PSE /* | CR4_PGE */ ;
	thread->reg_tsc = 0;

	/*
	 * Initialize the FPU/MMX/SSE state-save area.  Zeroing the area
	 * doesn't result in a valid state to be restored, so initialize it
	 * by resetting the actual registers and then saving them.  I think
	 * the current register state may be "live" for the calling partition,
	 * so we have to preserve it.  We initialize the state in either
	 * fxsave/fxrstor format or fsave/frstor format, depending on whether
	 * the processor supports fxsave/fxrstor.  The preempt path in
	 * preempt.S makes the same determination.
	 */
	assert((((uval)thread->fp_mmx_sse) & 0xf) == 0,
	       "thread->fp_mmx_sse not 16-byte aligned\n");
	assert((get_cr0() & (CR0_TS | CR0_EM)) == 0, "fpu not enabled\n");

	uval8 space[512 + 15];	/* extra space for alignment purposes */
	uval8 *tmp_fp_mmx_sse = (uval8 *)((((uval)space) + 15) & ~15);

	if ((cpuid_features & CPUID_FEATURES_FXSAVE) != 0) {
		/*
		 * The processor supports fxsave/fxrstor, which will do the
		 * right thing whether or not SSE/SSE2 are supported.
		 */
		/* *INDENT-OFF* */
		__asm__ __volatile__ (
			"fxsave %0; fwait\n"	/* preserve caller's state */
			"fninit\n"		/* reset state */
			"fxsave %1; fwait\n"	/* initialize thread state */
			"fxrstor %0"		/* restore caller's state */
			: "=m" (*tmp_fp_mmx_sse), "=m" (*(thread->fp_mmx_sse))
		);
		/* *INDENT-ON* */
		thread->reg_cr4 |= CR4_OSFXSR;
	} else {
		/*
		 * The processor does not support fxsave/fxrstor, so
		 * presumably SSE/SSE2 are not supported either and
		 * saving/restoring the FPU/MMX state is sufficient.
		 */
		/* *INDENT-OFF* */
		__asm__ __volatile__ (
			"fnsave %0; fwait\n"	/* preserve caller's state */
						/* fnsave resets state */
			"fnsave %1; fwait\n"	/* initialize thread state */
			"frstor %0"		/* restore caller's state */
			: "=m" (*tmp_fp_mmx_sse), "=m" (*(thread->fp_mmx_sse))
		);
		/* *INDENT-ON* */
	}

	if (cpuid_features & CPUID_FEATURES_XMM)
		thread->reg_cr4 |= CR4_OSXMMEXCPT;

	/* initialize the page table cache */
	pgc_init(os);

	/*
	 * Create the initial mappings for this thread. Each
	 * thread is started with the hypervisor mapped in and
	 * with the first chunk mapped 1-1.
	 */
	/* root of shadow page table directory */
	thread->pgd = pgc_alloc(thread, INVALID_PHYSICAL_ADDRESS, PGC_PGDIR);
	assert(thread->pgd != NULL, "Cannot create shadow page directory");

	thread->flushed = NULL;

	/* map in hypervisor and physical memory */
	pgc_initial_mapping(thread);

	thread->tss.cr3 = thread->pgd->pgdir.hv_paddr;

	thread->cb = NULL;
	thread->mbox = NULL;

	hprintf("arch_os_thread: thread lpid 0x%x, cr3 0x%lx\n",
		os->po_lpid, thread->tss.cr3);
}

void
arch_os_destroy(struct os *os)
{
	struct cpu_thread *thread = os->cpu[0]->thread;

	free_pages(&phys_pa, thread->tss.esp0, HV_STACK_SIZE);

	lpidtag_release(os);

	if (thread->mbox != NULL)
		hv_unmap(ALIGN_DOWN((uval)thread->mbox, PGSIZE), PGSIZE);
	pgc_destroy(os);
}

struct cpu_thread *
get_cur_thread(void)
{
	return (struct cpu_thread *)get_cur_tss()->threadp;
}

struct persist_mapping {
	struct lpar_event pm_base;
	uval pm_virt_addr;
	uval pm_size;
};

static uval
persist_mapping_cleanup(struct os *os, struct lpar_event *le, uval event)
{
	(void)os;
	if (event != LPAR_DIE) {
		return 1;
	}

	struct persist_mapping *pm = PARENT_OBJ(typeof(*pm), pm_base, le);

	hv_unmap(pm->pm_virt_addr, pm->pm_size);

	hfree(pm, sizeof (*pm));

	return 0;
}

uval
hv_map_LA(struct os *os, uval laddr, uval size)
{
	uval paddr = logical_to_physical_address(os, laddr, size);

	if (paddr == INVALID_PHYSICAL_ADDRESS) {
		return INVALID_LOGICAL_ADDRESS;
	}

	/* For now, we can only guarantee the existence of the first chunk
	 * for the the lifetime of the partition, other addresses may
	 * be unmapped. */
	if (laddr > CHUNK_SIZE) {
		return INVALID_LOGICAL_ADDRESS;
	}

	uval offset = paddr & ~PGMASK;

	paddr &= PGMASK;
	size += offset;

	struct persist_mapping *pm = halloc(sizeof (struct persist_mapping));

	/* tlb flush may not be necessary, hv_map should be smart
	 * enough to know for sure by looking at existing PTE's */
	pm->pm_virt_addr = hv_map(0, paddr, size, PTE_RW, 1);
	pm->pm_size = size;
	pm->pm_base.le_func = persist_mapping_cleanup;

	if (pm->pm_virt_addr == PAGE_ALLOC_ERROR) {
		hfree(pm, sizeof (*pm));
		return INVALID_PHYSICAL_ADDRESS;
	}
	register_event(os, &pm->pm_base);
	return pm->pm_virt_addr + offset;
}
