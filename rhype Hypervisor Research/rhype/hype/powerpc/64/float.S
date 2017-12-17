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
#include <os.h>
#include <asm_defs.h>

#ifdef HAS_FP			
save_fp:	
	stfd	f0,CT_FPR0(r3)
	stfd	f1,CT_FPR1(r3)
	stfd	f2,CT_FPR2(r3)
	stfd	f3,CT_FPR3(r3)
	stfd	f4,CT_FPR4(r3)
	stfd	f5,CT_FPR5(r3)
	stfd	f6,CT_FPR6(r3)
	stfd	f7,CT_FPR7(r3)
	stfd	f8,CT_FPR8(r3)
	stfd	f9,CT_FPR9(r3)
	stfd	f10,CT_FPR10(r3)
	stfd	f11,CT_FPR11(r3)
	stfd	f12,CT_FPR12(r3)
	stfd	f13,CT_FPR13(r3)
	stfd	f14,CT_FPR14(r3)
	stfd	f15,CT_FPR15(r3)
	stfd	f16,CT_FPR16(r3)
	stfd	f17,CT_FPR17(r3)
	stfd	f18,CT_FPR18(r3)
	stfd	f19,CT_FPR19(r3)
	stfd	f20,CT_FPR20(r3)
	stfd	f21,CT_FPR21(r3)
	stfd	f22,CT_FPR22(r3)
	stfd	f23,CT_FPR23(r3)
	stfd	f24,CT_FPR24(r3)
	stfd	f25,CT_FPR25(r3)
	stfd	f26,CT_FPR26(r3)
	stfd	f27,CT_FPR27(r3)
	stfd	f28,CT_FPR28(r3)
	stfd	f29,CT_FPR29(r3)
	stfd	f30,CT_FPR30(r3)
	stfd	f31,CT_FPR31(r3)
	mffs	f0
	stfd	f0,CT_FPSCR(r3)
	blr

restore_fp:	
	lfd	f0,CT_FPSCR(r3)
	mtfsf	0xff,f0

	lfd	f0,CT_FPR0(r3)
	lfd	f1,CT_FPR1(r3)
	lfd	f2,CT_FPR2(r3)
	lfd	f3,CT_FPR3(r3)
	lfd	f4,CT_FPR4(r3)
	lfd	f5,CT_FPR5(r3)
	lfd	f6,CT_FPR6(r3)
	lfd	f7,CT_FPR7(r3)
	lfd	f8,CT_FPR8(r3)
	lfd	f9,CT_FPR9(r3)
	lfd	f10,CT_FPR10(r3)
	lfd	f11,CT_FPR11(r3)
	lfd	f12,CT_FPR12(r3)
	lfd	f13,CT_FPR13(r3)
	lfd	f14,CT_FPR14(r3)
	lfd	f15,CT_FPR15(r3)
	lfd	f16,CT_FPR16(r3)
	lfd	f17,CT_FPR17(r3)
	lfd	f18,CT_FPR18(r3)
	lfd	f19,CT_FPR19(r3)
	lfd	f20,CT_FPR20(r3)
	lfd	f21,CT_FPR21(r3)
	lfd	f22,CT_FPR22(r3)
	lfd	f23,CT_FPR23(r3)
	lfd	f24,CT_FPR24(r3)
	lfd	f25,CT_FPR25(r3)
	lfd	f26,CT_FPR26(r3)
	lfd	f27,CT_FPR27(r3)
	lfd	f28,CT_FPR28(r3)
	lfd	f29,CT_FPR29(r3)
	lfd	f30,CT_FPR30(r3)
	lfd	f31,CT_FPR31(r3)
	blr
#endif /* HAS_FP */

#ifdef HAS_VMX
/* So you might think that we could use the VRSAVE register to
 * streamline this but "If this [VRSAVE] approach is taken it must be
 * applied rigorously".
 *	
 * Since we cannot rely on partitions to use this correctly
 * or at all we must save/restore them all, including vrsave
 */
save_vmx:
	mfspr	r0,SPRN_VRSAVE
	stw	r0,CT_VRSAVE(r3)

	/* r0 as the second operand is considered 0 */	
	addi r0,r3,CT_VR0 ;	stvxl	vr0,r0,r0
	addi r0,r3,CT_VR1 ;	stvxl	vr1,r0,r0
	addi r0,r3,CT_VR2 ;	stvxl	vr2,r0,r0
	addi r0,r3,CT_VR3 ;	stvxl	vr3,r0,r0
	addi r0,r3,CT_VR4 ;	stvxl	vr4,r0,r0
	addi r0,r3,CT_VR5 ;	stvxl	vr5,r0,r0
	addi r0,r3,CT_VR6 ;	stvxl	vr6,r0,r0
	addi r0,r3,CT_VR7 ;	stvxl	vr7,r0,r0
	addi r0,r3,CT_VR8 ;	stvxl	vr8,r0,r0

	/*
	 * By now vr0 should be pushed out so now is a good time to
	 * get the VRSCR which can take a long time and has no dependcies
	 * on the following operations.
	 */
	mfvscr	vr0
	addi r0,r3,CT_VSCR ;	stvxl	vr0,r0,r0
	
	addi r0,r3,CT_VR9 ;	stvxl	vr9,r0,r0
	addi r0,r3,CT_VR10 ;	stvxl	vr10,r0,r0
	addi r0,r3,CT_VR11 ;	stvxl	vr11,r0,r0
	addi r0,r3,CT_VR12 ;	stvxl	vr12,r0,r0
	addi r0,r3,CT_VR13 ;	stvxl	vr13,r0,r0
	addi r0,r3,CT_VR14 ;	stvxl	vr14,r0,r0
	addi r0,r3,CT_VR15 ;	stvxl	vr15,r0,r0
	addi r0,r3,CT_VR16 ;	stvxl	vr16,r0,r0
	addi r0,r3,CT_VR17 ;	stvxl	vr17,r0,r0
	addi r0,r3,CT_VR18 ;	stvxl	vr18,r0,r0
	addi r0,r3,CT_VR19 ;	stvxl	vr19,r0,r0
	addi r0,r3,CT_VR20 ;	stvxl	vr20,r0,r0
	addi r0,r3,CT_VR21 ;	stvxl	vr21,r0,r0
	addi r0,r3,CT_VR22 ;	stvxl	vr22,r0,r0
	addi r0,r3,CT_VR23 ;	stvxl	vr23,r0,r0
	addi r0,r3,CT_VR24 ;	stvxl	vr24,r0,r0
	addi r0,r3,CT_VR25 ;	stvxl	vr25,r0,r0
	addi r0,r3,CT_VR26 ;	stvxl	vr26,r0,r0
	addi r0,r3,CT_VR27 ;	stvxl	vr27,r0,r0
	addi r0,r3,CT_VR28 ;	stvxl	vr28,r0,r0
	addi r0,r3,CT_VR29 ;	stvxl	vr29,r0,r0
	addi r0,r3,CT_VR30 ;	stvxl	vr30,r0,r0
	addi r0,r3,CT_VR31 ;	stvxl	vr31,r0,r0
	blr
	
restore_vmx:
	lwz	r0,CT_VRSAVE(r3)
	mtspr	SPRN_VRSAVE,r0

	/*
	 * This operation can take a long time so we use vr31 to
	 * eliminate the depency on r0 for the next load
	 */
	addi r0,r3,CT_VSCR ;	lvxl	vr31,r0,r0
	mtvscr	vr31

	/* r0 as the second operand is considered 0 */	
	addi r0,r3,CT_VR0 ;	lvxl	vr0,r0,r0
	addi r0,r3,CT_VR1 ;	lvxl	vr1,r0,r0
	addi r0,r3,CT_VR2 ;	lvxl	vr2,r0,r0
	addi r0,r3,CT_VR3 ;	lvxl	vr3,r0,r0
	addi r0,r3,CT_VR4 ;	lvxl	vr4,r0,r0
	addi r0,r3,CT_VR5 ;	lvxl	vr5,r0,r0
	addi r0,r3,CT_VR6 ;	lvxl	vr6,r0,r0
	addi r0,r3,CT_VR7 ;	lvxl	vr7,r0,r0
	addi r0,r3,CT_VR8 ;	lvxl	vr8,r0,r0
	addi r0,r3,CT_VR9 ;	lvxl	vr9,r0,r0
	addi r0,r3,CT_VR10 ;	lvxl	vr10,r0,r0
	addi r0,r3,CT_VR11 ;	lvxl	vr11,r0,r0
	addi r0,r3,CT_VR12 ;	lvxl	vr12,r0,r0
	addi r0,r3,CT_VR13 ;	lvxl	vr13,r0,r0
	addi r0,r3,CT_VR14 ;	lvxl	vr14,r0,r0
	addi r0,r3,CT_VR15 ;	lvxl	vr15,r0,r0
	addi r0,r3,CT_VR16 ;	lvxl	vr16,r0,r0
	addi r0,r3,CT_VR17 ;	lvxl	vr17,r0,r0
	addi r0,r3,CT_VR18 ;	lvxl	vr18,r0,r0
	addi r0,r3,CT_VR19 ;	lvxl	vr19,r0,r0
	addi r0,r3,CT_VR20 ;	lvxl	vr20,r0,r0
	addi r0,r3,CT_VR21 ;	lvxl	vr21,r0,r0
	addi r0,r3,CT_VR22 ;	lvxl	vr22,r0,r0
	addi r0,r3,CT_VR23 ;	lvxl	vr23,r0,r0
	addi r0,r3,CT_VR24 ;	lvxl	vr24,r0,r0
	addi r0,r3,CT_VR25 ;	lvxl	vr25,r0,r0
	addi r0,r3,CT_VR26 ;	lvxl	vr26,r0,r0
	addi r0,r3,CT_VR27 ;	lvxl	vr27,r0,r0
	addi r0,r3,CT_VR28 ;	lvxl	vr28,r0,r0
	addi r0,r3,CT_VR29 ;	lvxl	vr29,r0,r0
	addi r0,r3,CT_VR30 ;	lvxl	vr30,r0,r0
	addi r0,r3,CT_VR31 ;	lvxl	vr31,r0,r0
	blr
	
#endif /* HAS_VMX */

	
### switch_float(per_cpu_os *pcop, per_cpu_os *nextpcop)
C_TEXT_ENTRY(switch_float)
	mflr	r8
#ifdef HAS_FP
	mfmsr	r9		# save msr
	ori	r0,r9,MSR_FP	# turn on FPU
	mtmsr	r0
	bl	save_fp		# uses r3
	mr      r3,r4
	bl	restore_fp	# uses r3
	mtmsr	r9		# restore msr
#endif /* HAS_FP */
#ifdef HAS_VMX
	mfmsr	r9		# save msr
	oris	r0,r9,(MSR_VMX>>16)	# turn on VMX
	mtmsr	r0
	bl	save_vmx	# uses r3
        mr      r3,r4	
	bl	restore_vmx	# uses r3
	mtmsr	r9		# restore msr
#endif /* HAS_VMX */
	mtlr	r8
	blr

C_TEXT_ENTRY(save_float)
	mflr	r8
#ifdef HAS_FP
	mfmsr	r9		# save msr
	ori	r0,r9,MSR_FP	# turn on FPU
	mtmsr	r0
	bl	save_fp		# uses r3
	mtmsr	r9		# restore msr
#endif /* HAS_FP */
#ifdef HAS_VMX
	mfmsr	r9		# save msr
	oris	r0,r9,(MSR_VMX>>16)	# turn on VMX
	mtmsr	r0
	bl	save_vmx	# uses r3
	mtmsr	r9		# restore msr
#endif /* HAS_VMX */
	mtlr	r8
	blr

C_TEXT_ENTRY(restore_float)
	mflr	r8
#ifdef HAS_FP
	mfmsr	r9		# save msr
	ori	r0,r9,MSR_FP	# turn on FPU
	mtmsr	r0
	bl	restore_fp	# uses r3
	mtmsr	r9		# restore msr
#endif /* HAS_FP */
#ifdef HAS_VMX
	mfmsr	r9		# save msr
	oris	r0,r9,(MSR_VMX>>16)	# turn on VMX
	mtmsr	r0
	bl	restore_vmx	# uses r3
	mtmsr	r9		# restore msr
#endif /* HAS_VMX */
	mtlr	r8
	blr
