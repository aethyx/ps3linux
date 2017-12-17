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
#include <asm.h>
#include <pmm.h>
#include <partition.h>
#include <hype.h>
#include <slb.h>
#include <mmu.h>
#include <float.h>
#include <thread_control_area.h>
#include <machine_config.h>
#include <llan.h>
#include <eic.h>
#include <hv_regs.h>
#include <debug.h>
#include <crq.h>
#include <lpidtag.h>

uval
arch_os_init(struct os *newOS, uval pinfo_addr)
{
	struct partition_info *pinfo;
	struct cpu_thread *newThread;
	struct cpu *newCpu;
	int mycpu;
	int mythread;

	struct logical_chunk_info* lci = laddr_to_lci(newOS, 0);
	uval base_size = lci->lci_size;
	uval basemem = logical_to_physical_address(newOS, 0, base_size);

	if (basemem == INVALID_PHYSICAL_ADDRESS) {
		return (uval)H_Parameter;
	}

	if (!cpu_verify_rmo(basemem, base_size)) {
		return (uval)H_Parameter;
	}
	newOS->rmo = basemem;
	newOS->rmo_size = base_size;

	newOS->po_boot_msr = MSR_ME;
	newOS->po_exception_msr = MSR_ME;
	newOS->po_exception_vec = 0;

	newOS->use_large_pages = 1;

	if (pinfo_addr != INVALID_LOGICAL_ADDRESS) {
		pinfo_addr += basemem;
		pinfo = (struct partition_info *)pinfo_addr;
		hprintf("partition info at 0x%lx\n", pinfo_addr);

		newOS->os_partition_info = pinfo;
		memcpy((newOS->cached_partition_info),
		       pinfo, 2 * sizeof (struct partition_info));
	} else {
		pinfo = &newOS->cached_partition_info[0];
		newOS->os_partition_info = pinfo;

		memset(&newOS->cached_partition_info, 0,
		       2 * sizeof (struct partition_info));

		newOS->cached_partition_info[0].large_page_size1 =
			LARGE_PAGE_SIZE16M;

		newOS->cached_partition_info[0].large_page_size2 =
			DEFAULT_LARGE_PAGE_SIZE2;

		newOS->cached_partition_info[1].large_page_size1 =
			newOS->cached_partition_info[0].large_page_size1;
		newOS->cached_partition_info[1].large_page_size2 =
			newOS->cached_partition_info[0].large_page_size2;
	}

	pinfo->lpid = newOS->po_lpid;

	/* 0b0010 = 16M & 64K */
	newOS->large_page_selection =
		(get_large_page_hid_bits(pinfo->large_page_size1) << 2) |
		get_large_page_hid_bits(pinfo->large_page_size2);

	pinfo[1].large_page_size1 = LARGE_PAGE_SIZE16M;
	pinfo[1].large_page_size2 = DEFAULT_LARGE_PAGE_SIZE2;

	newOS->cached_partition_info[1].large_page_size1 = LARGE_PAGE_SIZE16M;
	newOS->cached_partition_info[1].large_page_size2 =
		DEFAULT_LARGE_PAGE_SIZE2;

#ifdef SWTLB
	newOS->cached_partition_info[1].sfw_tlb = pinfo[0].sfw_tlb;
	pinfo[1].sfw_tlb = pinfo[0].sfw_tlb;
#else
	newOS->cached_partition_info[1].sfw_tlb = 0;
	pinfo[1].sfw_tlb = 0;
#endif

	newOS->large_page_shift[0] =
		get_large_page_shift(pinfo->large_page_size1);
	newOS->large_page_shift[1] =
		get_large_page_shift(pinfo->large_page_size2);

	lpidtag_acquire(newOS);
	vtty_init(newOS);

	assert(newOS->cpu != NULL, "OS not assigned to a CPU\n");
	for (mycpu = 0; mycpu < MAX_CPU; mycpu++) {
		newCpu = newOS->cpu[mycpu];

#ifdef HAS_TAGGED_TLB
		/*
		 * need to flush the TLB in case there are entries for
		 * this LPID left over from a previous OS
		 */
		newCpu->tlb_flush_pending = 1;
#endif

		for (mythread = 0; mythread < THREADS_PER_CPU; ++mythread) {
			newThread = newCpu->thread + mythread;

			newThread->state = CT_STATE_OFFLINE;

		}
	}

	return core_os_init(newOS);
}

void
arch_os_destroy(struct os *os)
{
	lpidtag_release(os);
}

/* Set current thread to resume at exception vector by setting the
 * srr* and hsrr* registers.
 */
static void
set_to_exception(struct cpu_thread *thread, uval vector,
		 uval curr_pc, uval curr_msr)
{
	(void)thread;		/* Will use if need to support non-HV HW */

	const uval srr_mask = ~(MSR_IR | MSR_DR | MSR_FE0 | MSR_PR |
				MSR_FE1 | MSR_EE | MSR_RI);

	/* hsrr0/1 -> srr0/1 */
	uval srr0 = curr_pc;

	/* SRR1[33:36] & SRR1[42:47] set to 0 */
	uval srr1 = curr_msr & ~0x00000000783f0000;

	/* external interrupt -> hsrr0/1 */
	uval hsrr0 = vector;

	uval hsrr1 = curr_msr & srr_mask;

	hsrr1 = srr1_set_sf(hsrr1);

	mtsrr0(srr0);
	mtsrr1(srr1);
	mthsrr0(hsrr0);
	mthsrr1(hsrr1);
}

#ifdef HAS_MEDIATED_EE
static void
deliver_MER(struct cpu_thread *thread, uval exc_pc, uval exc_msr)
{
	uval vector;

	/* a mediated interrupt is present - drive an external
	 * to the OS if needed */
	int external_present = !xirqs_empty(thread);

	assert(thread->imp_regs.lpcr.bits.m == 1,
	       "got MER when it is not enabled\n");

	assert(exc_msr & MSR_EE, "got MER and EE is not on\n");

	DEBUG_MT(DBG_INTRPT, "%s: LPAR[0x%x] mediated external.\n",
		 __func__, thread->cpu->os->po_lpid);

	if (external_present) {
		vector = 0x500;

		DEBUG_MT(DBG_INTRPT, "%s: delivering external direct "
			 "to OS @ 0x%lx\n", __func__, vector);

		assert((exc_msr & MSR_EE), "how do we get a MER w/o EE on?\n");

		set_to_exception(thread, vector, exc_pc, exc_msr);
	} else {
		/* turn off LPCR[MER] since externals are no
		 * longer present */

		DEBUG_MT(DBG_INTRPT, "%s: MER was on with no externals pending\n",
			 __func__);
	}

	/* disable mediated interrupts */
	thread_set_MER(thread, 0);
}
#endif /* HAS_MEDIATED_EE */

void
handle_external(struct cpu_thread *thread)
{

	/* We must return from here with hsrr0/hsrr1 set to the pc/msr
	 * that we wish to restore this partition to. We may
	 * reschedule to a different lpar after returning, but assume
	 * here that we don't.
	 */
	struct cpu_thread *target_thread;

	uval lpar_msr;
	uval lpar_pc;

	DEBUG_MT(DBG_INTRPT, "handle_external: enter\n", 0);

#ifndef HAS_MEDIATED_EE
	lpar_pc = mfsrr0();
	lpar_msr = mfsrr1();
#else
	/* WARNING: assuming that LPES0 == 0, otherwise use srr* */
	lpar_pc = mfhsrr0();
	lpar_msr = mfhsrr1();

	if (lpar_msr & MSR_MER) {
		deliver_MER(thread, lpar_pc, lpar_msr);
		return;
	}
#endif /* HAS_MEDIATED_EE */

	target_thread = eic_exception(thread);

	if (target_thread == thread) {
		/* the interrupt is targeted for this thread -
		 * deliver it if possible */
		if (lpar_msr & MSR_EE) {
			DEBUG_MT(DBG_INTRPT, "delivering external interrupt "
				 "to current thread - direct\n", 0);
			/* the thread is currently enabled -
			 * deliver an external */

			set_to_exception(thread, 0x500, lpar_pc, lpar_msr);
		} else {
			DEBUG_MT(DBG_INTRPT, "delivering external "
				 "interrupt to current thread - MER\n", 0);
			/* the thread is not currently enabled
			 * - turn on MER */

			thread_set_MER(thread, 1);
		}
		return;
	}

	if (target_thread != NULL) {
		DEBUG_MT(DBG_INTRPT, "external interrupt is not for this "
			 "thread - preempting\n", 0);
		/* the interrupt is targeted for a different
		 * thread - preempt this thread */
		/* TODO - this needs to me made SMT aware -
		 * could the target be our partner thread? */
		/* TODO - the scheduler needs to be made aware
		 * that target_thread should be run next */

		/* Don't flag preemption  if target is not on this cpu */
		if (is_colocated(thread, target_thread) >= CPU_MATCH) {
			thread->preempt = PREEMPT_SELF;
		}
	}
	/* it is possible that the interrupt has no target and the HV
	 * ate it, in which case we simply return */
#ifndef HAS_MEDIATED_EE
	mthsrr0(lpar_pc);
	mthsrr1(lpar_msr);
#endif
	return;
}

/*
 * This function is called before we start processing exceptions to
 * see if/how they can be delivered to a partition. For PPC, we clear
 * "mediated EE" if EE is on (since clearly at this point there is no
 * need to defer interrupts, EE is on , we can deliver them now).
 */
uval
clear_exception_state(struct cpu_thread *thread)
{
#ifdef	HAS_MEDIATED_EE
	if (thread->reg_hsrr1 & MSR_EE) {
		thread_set_MER(thread, 0);
	}
#else
	thread = thread;
#endif /* HAS_MEDIATED_EE */

	return 0;
}

uval
force_intr_exception(struct cpu_thread *thread, uval ex_vector)
{
	const uval srr1_mask = ~((0xfUL << (63 - 36)) | 0x3fUL << (63 - 47));

	if ((thread->reg_hsrr1 & MSR_EE) == 0) {
		/* cannot deliver it yet */
#ifdef HAS_MEDIATED_EE
		if (thread_set_MER(thread, 1) == 0) {
			DEBUG_MT(DBG_INTRPT, "LPAR[0x%x]: interrupt mediated.\n",
				 thread->cpu->os->po_lpid);
		}
#else  /* HAS_MEDIATED_EE */
		DEBUG_MT(DBG_INTRPT, "LPAR[0x%x]: interrupt deferred.\n",
			 thread->cpu->os->po_lpid);
#endif /* HAS_MEDIATED_EE */
		return 0;
	}

	thread->reg_srr1 = thread->reg_hsrr1 & srr1_mask;
	thread->reg_srr0 = thread->reg_hsrr0;

	uval msr = thread->reg_hsrr1;

	thread->reg_hsrr0 = ex_vector;

	thread->reg_hsrr1 = 0;
	thread->reg_hsrr1 |= (msr & MSR_ME);
	thread->reg_hsrr1 |= (msr & MSR_ILE);
	thread->reg_hsrr1 |= ((msr & MSR_ILE) ? MSR_LE : 0);
#ifdef HAS_MSR_IP
	thread->reg_hsrr1 |= (msr & MSR_IP);
#endif

	return 0;
}

void
switch_large_page_support(struct cpu_thread *cur, struct cpu_thread *next)
{
	if (cur == NULL ||
	    (next->cpu->os->use_large_pages != cur->cpu->os->use_large_pages)
	    || (next->cpu->os->large_page_selection !=
		cur->cpu->os->large_page_selection)) {
		restore_large_page_selection(next);
	}
}

uval
arch_preempt_os(struct cpu_thread *curThread, struct cpu_thread *nextThread)
{
	switch_slb(curThread, nextThread);
	mtsdr1(nextThread->cpu->reg_sdr1);
	switch_float(curThread, nextThread);
	switch_large_page_support(curThread, nextThread);
	tlb_switch (nextThread->cpu);

	return 0;
}

void
arch_init_thread(struct os *os, struct cpu_thread *thread,
		 uval pc, uval reg2 /* toc */ ,
		 uval reg3, uval reg4, uval reg5, uval reg6, uval reg7)
{

	thread->reg_hsrr1 = os->po_boot_msr;

	thread->reg_gprs[1] = os->rmo_size - BOOT_STACK_SIZE;
	thread->reg_gprs[2] = reg2;
	thread->reg_hsrr0 = pc;

	thread->reg_gprs[3] = reg3;
	thread->reg_gprs[4] = reg4;
	thread->reg_gprs[5] = reg5;
	thread->reg_gprs[6] = reg6;
	thread->reg_gprs[7] = reg7;

	/* Set up LPCR, RMOR or equivalents */
	imp_thread_init(thread);

	if (reg2 == 0) {
		/* HACK ALERT we are 32 bit.. HACK ALERT */
		uval mask32 = MSR_SF;

#ifdef HAS_MSR_ISF
		mask32 |= MSR_ISF;
#endif
		thread->reg_hsrr1 &= ~mask32;
	}

	/* no need for lock here - we can increase the state without
	 * the lock */
	thread->state = CT_STATE_EXECUTING;

}

struct cpu_thread *
get_cur_thread(void)
{
	return ((struct thread_control_area *)get_tca())->active_thread;
}
