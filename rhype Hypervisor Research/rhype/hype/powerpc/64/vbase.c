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
#include <types.h>
#include <cpu_thread.h>
#include <pmm.h>
#include <vregs.h>
#include <xh.h>
#include <bitops.h>
#include <hype_control_area.h>
#include <thread_control_area.h>
#include <objalloc.h>
#include <vm_class.h>
#include <vm_class_inlines.h>
#include <vdec.h>
#include <vm.h>



#define FORCE_4K_PAGES 1

sval
vbase_config(struct cpu_thread* thr, uval vbase, uval size)
{
	uval i = 0;
	struct vstate *v;
	thr->vbo = vbase;
	thr->vbl = size;

	struct vm_class *vmc;
	thr->vmc_vregs = vmc_create_vregs();


	thr->vregs = (struct vregs*)thr->vmc_vregs->vmc_data;
	memset(thr->vregs, 0, sizeof(typeof(*thr->vregs)));
	thr->vregs->v_pir = mfpir();
	v = &thr->vstate;
	v->thread_mode = VSTATE_KERN_MODE;


	vmc = vmc_create_linear(0, 0, 1ULL << VM_CLASS_SPACE_SIZE_BITS, 0);
	thr->cpu->os->vmc_kernel[0] = vmc;


	for (i = 0 ; i < NUM_MAP_CLASSES; ++i) {
		thr->vregs->v_active_cls[i] = INVALID_CLASS;
	}

	partition_set_dec(thr, -1);

	return 0;
}



uval
xh_syscall(uval a1, uval a2, uval a3, uval a4, uval a5, uval a6,
	   uval a7, uval a8, uval a9, uval a10);

typedef	sval (*hcall_fn_t)(struct cpu_thread* thr, uval a2, uval a3, uval a4,
			   uval a5, uval a6, uval a7, uval a8,
			   uval a9, uval a10);


sval xh_kern_slb(struct cpu_thread* thread, uval addr,
		 struct vexc_save_regs *vr);
sval xh_kern_prog_ex(struct cpu_thread* thread, uval addr);
sval xh_kern_pgflt(struct cpu_thread* thread, uval addr,
		   struct vexc_save_regs *vr);

sval
htab_purge_vsid(struct cpu_thread *thr, uval vsid, uval num_vsids)
{
	uval end = thr->cpu->os->htab.num_ptes;
	uval i = 0;
	union pte *ht = (union pte*)GET_HTAB(thr->cpu->os);
	for (; i < end; ++i) {
		/* vsid is 52 bits, avpn has 57 */
		uval pte_vsid = ht[i].bits.avpn >> 5;
		if (ht[i].bits.v == 0 || pte_vsid - vsid >= num_vsids) {
			continue;
		}

		pte_lock(&ht[i]);
		if (ht[i].bits.v == 0 || pte_vsid - vsid >= num_vsids) {
			pte_unlock(&ht[i]);
			continue;
		}

		pte_invalidate(&thr->cpu->os->htab, &ht[i]);
		pte_unlock(&ht[i]);
		ptesync();
		do_tlbie(&ht[i], i);
	}
	return H_Success;
}


sval
insert_ea_map(struct cpu_thread *thr, uval vsid, uval ea, union ptel entry)
{
	int j = 0;
	uval lp = LOG_PGSIZE;  /* FIXME: get large page size */
	uval vaddr = (vsid << LOG_SEGMENT_SIZE) | (ea & (SEGMENT_SIZE - 1));
	uval log_ht_size = bit_log2(thr->cpu->os->htab.num_ptes)+ LOG_PTE_SIZE;
	uval pteg = NUM_PTES_IN_PTEG * get_pteg(vaddr, vsid, lp, log_ht_size);

	union pte *ht = (union pte *)GET_HTAB(thr->cpu->os);
	union pte pte;

	pte.words.vsidWord = 0;
	pte.words.rpnWord = entry.word;

	pte.bits.avpn = (vsid << 5) | VADDR_TO_API(vaddr);
	pte.bits.v = 1;

	union pte *target = &ht[pteg];
	int empty = NUM_PTES_IN_PTEG;
	uval remove = 0;

redo_search:

	for (; j < NUM_PTES_IN_PTEG; ++j, ++target) {
		if (target->bits.avpn == pte.bits.avpn) {
			empty = j;
			remove = 1;

			break;
		}
		if (j < empty && target->bits.v == 0) {
			empty = j;
		}
	}


	assert(empty != NUM_PTES_IN_PTEG, "Full htab\n");

	target = &ht[pteg + empty];
	pte_lock(target);

	/* Things may have changed since we got the lock */
	if (target->bits.v && target->bits.avpn != pte.bits.avpn)
		goto redo_search;


	if (remove) {
		pte_invalidate(&thr->cpu->os->htab, target);
		ptesync();
		do_tlbie(target, pteg + empty);
	}

//	hprintf("insert: ea: 0x%lx vsid: 0x%lx -> idx: 0x%lx\n",
//		ea, vsid, j + pteg);
	if (pte.bits.v) {
		pte.bits.lock = 1; /* Make sure new entry is still locked */
		pte_insert(&thr->cpu->os->htab, empty + pteg,
			   pte.words.vsidWord, pte.words.rpnWord, ea);
	}
	pte_unlock(target);

	return H_Success;
}


sval
xh_kern_pgflt(struct cpu_thread* thr, uval type, struct vexc_save_regs *vr)
{
	struct vm_class *vmc = NULL;
	uval orig_addr;
	struct thread_control_area *tca = get_tca();

	if (type == 1) {
		orig_addr = mfdar();
	} else {
		orig_addr = tca->srr0;
	}


	if (thr->vstate.thread_mode & VSTATE_KERN_MODE) {
		vmc = find_kernel_vmc(thr, orig_addr);
	}

	if (!vmc) {
		vmc = find_app_vmc(thr, orig_addr);
	}

	if (!vmc) {
		hprintf("No vm_class for 0x%lx\n", orig_addr);
		breakpoint();

		return insert_debug_exception(thr, V_DEBUG_MEM_FAULT);
	}

	uval addr = ALIGN_DOWN(orig_addr, PGSIZE);
	union ptel pte = { .word = 0 };
	uval la = vmc_xlate(vmc, addr, &pte);
	uval ra;
	uval vsid;

	if (la == INVALID_LOGICAL_ADDRESS) {
		/* If logical address is invalid, and pte is non-zero, then
		 * pte contains physical address
		 */
		if (pte.word == 0) {
			goto reflect;
		}

		ra = pte.bits.rpn << LOG_PGSIZE;
	} else {
		ra = logical_to_physical_address(thr->cpu->os, la, PGSIZE);
	}

	vsid = vmc_class_vsid(thr, vmc, addr);

	pte.bits.rpn = ra >> LOG_PGSIZE;


	sval ret = insert_ea_map(thr, vsid, addr, pte);
	if (ret == H_Success) {

		return vr->reg_gprs[3];
	}

reflect:
	thr->vregs->v_dar = orig_addr;
	thr->vregs->v_dsisr = mfdsisr();


	assert(thr->vregs->exception_vectors[EXC_V_PGFLT],
	       "no pgflt vector defined\n");

	return insert_exception(thr, EXC_V_PGFLT);
}


sval
xh_kern_slb(struct cpu_thread* thread, uval type, struct vexc_save_regs *vr)
{
	struct vm_class *vmc = NULL;
	struct thread_control_area *tca = get_tca();
	uval addr;

	if (type == 1) {
		addr = mfdar();
	} else {
		addr = tca->srr0;
	}

	uval seg_base = ALIGN_DOWN(addr, SEGMENT_SIZE);
	uval lp = LOG_PGSIZE;  /* FIXME: get large page size */
	uval l = 1;
	uval spot;

	if (thread->vstate.thread_mode & VSTATE_KERN_MODE) {
		vmc = find_kernel_vmc(thread, addr);
	}

	if (!vmc) {
		vmc = find_app_vmc(thread, addr);
	}

	if (!vmc) {
		hprintf("No vm_class for 0x%lx\n", addr);
		return insert_debug_exception(thread, V_DEBUG_MEM_FAULT);
	}


	uval vsid = vmc_class_vsid(thread, vmc, addr);

#ifdef FORCE_4K_PAGES
	lp = 12;
	l = 0;
	spot = slb_insert(seg_base, 0, 0, 1, vsid, thread->slb_entries);
#else
	spot = slb_insert(ea, 1, SELECT_LG, 1, vsid, thread->slb_entries);
#endif

	return vr->reg_gprs[3];
}

uval
xh_syscall(uval a1, uval a2, uval a3, uval a4, uval a5, uval a6,
	   uval a7, uval a8, uval a9, uval a10)
{
	struct thread_control_area* tca = (struct thread_control_area*)mfr13();
	struct cpu_thread* thread = tca->active_thread;
	hcall_fn_t hcall_fn;
	const hcall_vec_t* vec = (const hcall_vec_t*)hca.hcall_vector;
	thread->return_args = tca->save_area;

	a1 >>= 2;

	if (a1 >= hca.hcall_vector_len &&
	    a1 - 0x1800 < hca.hcall_6000_vector_len) {
		vec = (const hcall_vec_t*)hca.hcall_6000_vector;
		a1 -= 0x1800;
	}

	hcall_fn = *(const hcall_fn_t*)&vec[a1];
	return hcall_fn(thread, a2, a3, a4, a5, a6, a7, a8, a9, a10);
}

extern void insert_dec_exception(void);
extern void insert_ext_exception(void);

inline void
set_v_msr(struct cpu_thread* thr, uval val)
{
	struct thread_control_area *tca = get_tca();

	if ((val ^ thr->vregs->v_msr) & MSR_PR) {
		if (val & MSR_PR) {
			vmc_exit_kernel(thr);
			thr->vstate.thread_mode &= ~VSTATE_KERN_MODE;
		} else {
			vmc_enter_kernel(thr);
			thr->vstate.thread_mode |= VSTATE_KERN_MODE;
		}
		tca->vstate = thr->vstate.thread_mode;
	}

	thr->vregs->v_msr = (val & ~(MSR_HV|(MSR_SF>>2))) | MSR_AM;

	if (! (val & MSR_EE)) {
		return;
	}
	assert(get_tca()->restore_fn == NULL,
	       "Exception delivery already pending.\n");

	if (thr->vstate.thread_mode & VSTATE_PENDING_EXT) {
		get_tca()->restore_fn = insert_ext_exception;

	} else if (thr->vstate.thread_mode & VSTATE_PENDING_DEC) {
		get_tca()->restore_fn = insert_dec_exception;
	}
}

static inline void
mtgpr(struct cpu_thread* thr, uval gpr, uval val)
{
	switch (gpr) {
	case 14 ... 31:
		thr->reg_gprs[gpr] = val;
		break;
	case 0 ... 13:
		get_tca()->save_area->reg_gprs[gpr] = val;
		break;
	}
}

static inline uval
mfgpr(struct cpu_thread* thr, uval gpr)
{
	uval val;
	switch (gpr) {
	case 14 ... 31:
		val = thr->reg_gprs[gpr];
		break;
	case 0 ... 13:
		val = get_tca()->save_area->reg_gprs[gpr];
		break;
	}
	return val;

}

/* Extract bits of word, start/stop indexing uses Book I/III conventions
 * (i.e. bit 0 is on the left)
 */
static inline
uval extract_bits(uval val, uval start, uval numbits)
{
	return (val >> (32 - (start + numbits))) & ((1 << numbits) - 1);
}

static sval
decode_spr_ins(struct cpu_thread* thr, uval addr, uval32 ins)
{
	struct thread_control_area *tca = get_tca();
	uval id = thr->vregs->active_vsave;
	struct vexc_save_regs *vr = &thr->vregs->vexc_save[id];
	sval ret = -1;
	uval opcode = extract_bits(ins, 0, 6);
	uval type = extract_bits(ins, 21, 10);
	uval spr_0_4 = extract_bits(ins, 16, 5);
	uval spr_5_9 = extract_bits(ins, 11, 5);
	uval gpr  = extract_bits(ins, 6, 5);
	uval spr = (spr_0_4 << 5) | spr_5_9;

	/* mfmsr */
	if (opcode == 31 && type == 83) {
		//hprintf("mfmsr r%ld at 0x%lx\n",gpr, addr);
		mtgpr(thr, gpr, thr->vregs->v_msr);
		tca->srr0 += sizeof(uval32);
		return 0;
	}

	/* mtmsrd */
	if (opcode == 31 && (type == 178 || type == 146)) {
		uval64 val = mfgpr(thr, gpr);
		//hprintf("mtmsrd r%ld <- 0x%llx at 0x%lx\n", gpr, val, addr);

		uval64 chg_mask = ~0ULL;
		uval l = extract_bits(ins, 15, 1);
		if (type == 146) {
			// mtmsr , 32-bits
			chg_mask = 0xffffffff;
		}

		if (l == 1) {
			chg_mask = (MSR_EE | MSR_RI);
		}

		/* These are the only bits we can change here */
		val = (val & chg_mask) | (thr->vregs->v_msr & ~chg_mask);

		set_v_msr(thr, val);
		val = thr->vregs->v_msr;
		val |= V_LPAR_MSR_ON;
		val &= ~V_LPAR_MSR_OFF;
		tca->srr1 = val;
		tca->srr0 += sizeof(uval32);
		return 0;
	}

	/* mfspr */
#define SET_GPR(label, src)					\
	case label: mtgpr(thr, gpr, src); break;

	if (opcode == 31 && type == 339) {
		ret = 0;
		switch (spr) {
			SET_GPR(SPRN_SRR0, vr->v_srr0);
			SET_GPR(SPRN_SRR1, vr->v_srr1);
			SET_GPR(SPRN_PVR, mfpvr());
			SET_GPR(SPRN_PIR, mfpir());

		case SPRN_DSISR:
		case SPRN_DAR:
			mtgpr(thr, gpr, 0);
			break;
		case SPRN_HID0:
		case SPRN_HID1:
		case SPRN_HID4:
		case SPRN_HID5:
			mtgpr(thr, gpr, 0xdeadbeeffeedfaceULL);
			break;

		default:
			ret = -1;
			break;
		}

		if (ret != -1) {
			tca->srr0 += sizeof(uval32);
			return ret;
		}

	}

#define SET_VREG(label, field)						\
	case label: thr->vregs->field = mfgpr(thr, gpr); break;

	/* mtspr */
	if (opcode == 31 && type == 467) {
		ret = 0;
		switch (spr) {
			SET_VREG(SPRN_SPRG0, v_sprg0);
			SET_VREG(SPRN_SPRG1, v_sprg1);
			SET_VREG(SPRN_SPRG2, v_sprg2);
			SET_VREG(SPRN_SPRG3, v_sprg3);
		case SPRN_DEC:
			partition_set_dec(thr, mfgpr(thr, gpr));
			thr->vregs->v_dec = mfgpr(thr, gpr);
			break;
		case SPRN_SRR0:
			vr->v_srr0 = mfgpr(thr, gpr);
			break;
		case SPRN_SRR1:
			vr->v_srr1 = mfgpr(thr, gpr);
			break;

		case SPRN_DSISR:
		case SPRN_DAR:
			break;
		default:
			ret = -1;
			break;
		}

		if (ret != -1) {
			tca->srr0 += sizeof(uval32);
			return ret;
		}

	}

	/* rfid */
	if (opcode == 19 && type == 18) {

		uval val = vr->v_srr1;

		set_v_msr(thr, val);
		val |= V_LPAR_MSR_ON;
		val &= ~V_LPAR_MSR_OFF;
		tca->srr1 = val;
		tca->srr0 = vr->v_srr0;
		hprintf("rfid: %lx -> %lx\n",addr, vr->v_srr0);

		return 0;
	}

	if (ret == -1) {
		hprintf("Decode instruction: %ld %ld %ld %ld\n",
			opcode, type, spr, gpr);
	}
	return ret;
}

sval
xh_kern_prog_ex(struct cpu_thread* thread, uval addr)
{
	uval32 ins;
	struct thread_control_area *tca = get_tca();

	sval ret = 0;
	uval srr1 = tca->srr1;

	if (! (srr1 & (1ULL << (63-45)))) goto abort;

	struct vm_class *vmc = find_kernel_vmc(thread, addr);
	if (!vmc) goto abort;

	union ptel pte;
	uval laddr = vmc_xlate(vmc, addr, &pte);
	if (laddr == INVALID_LOGICAL_ADDRESS) goto abort;

	uval pa = logical_to_physical_address(thread->cpu->os, laddr,
					      sizeof(uval32));
	if (pa == INVALID_PHYSICAL_ADDRESS) goto abort;

	ins = *(uval32*)pa;

	ret = decode_spr_ins(thread, addr, ins);
	if (ret == 0) return -1;
abort:
	return insert_exception(thread, EXC_V_DEBUG);
}



typedef uval (*hype_xh_fn_t)(struct cpu_thread* thr);

static uval
default_xh(struct cpu_thread* thr)
{
	(void)thr;

	return 0;
}

hype_xh_fn_t vxh_table[] = {
	[XH_SYSRESET ... XH_FP] = &default_xh,
};


sval xh_ext(struct cpu_thread *thread);

sval
xh_ext(struct cpu_thread *thread)
{
	struct thread_control_area *tca = get_tca();
	hprintf("Inserting external exception\n");

	thread->vstate.thread_mode &= ~VSTATE_PENDING_EXT;
	tca->vstate &= ~VSTATE_PENDING_EXT;
	return insert_exception(thread, EXC_V_EXT);

}



sval
insert_exception(struct cpu_thread *thread, uval exnum)
{
	struct thread_control_area *tca = get_tca();
	struct vregs* vregs = tca->vregs;
	struct vexc_save_regs* vr = tca->save_area;

//	hprintf("Reflect exception: %ld %x\n", exnum, vr->exc_num);
	if (!thread->vregs->exception_vectors[exnum])
		return vregs->active_vsave;

	vr->v_srr0 = tca->srr0;

	/* Provide accurate trap bits */
	vr->v_srr1 = vregs->v_msr | (MSR_TRAP_BITS & tca->srr1);

	/* enforce HV, EE, PR off, AM on */
	uval vmsr = vregs->v_msr;

	vmsr &= ~(MSR_EE|MSR_HV|MSR_PR|MSR_IR|MSR_DR|MSR_FE0|MSR_FE1|MSR_BE|MSR_FP|MSR_PMM|MSR_SE|MSR_BE);
	vmsr |= MSR_SF;
//	hprintf("v_msr: 0x%016lx v_ex_msr: 0x%016lx %ld\n",
//		vregs->v_msr, vmsr, exnum);
	set_v_msr(thread, vmsr);

	uval64 msr = vregs->v_ex_msr;
	msr |= V_LPAR_MSR_ON;
	msr &= ~V_LPAR_MSR_OFF;
	tca->srr1 = msr;
	tca->srr0 = vregs->exception_vectors[exnum];

	/* Make vregs->v_dec match current dec value */
	sync_from_dec();


	vr->prev_vsave = vregs->active_vsave;
	vregs->active_vsave = (vr - &vregs->vexc_save[0]);

	return vregs->active_vsave;
}


sval
insert_debug_exception(struct cpu_thread *thread, uval dbgflag)
{
	struct thread_control_area *tca = get_tca();
	if (thread->vregs->debug & V_DEBUG_FLAG) {

		thread->vregs->debug |= dbgflag;

		/* Increment to the next instruction */
		tca->srr0 += sizeof(uval32);

		return thread->vregs->active_vsave;
	}
	return insert_exception(thread, EXC_V_DEBUG);

}

