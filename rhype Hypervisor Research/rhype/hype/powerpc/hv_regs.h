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
 *
 */
#ifndef _POWERPC_HV_REGS_H
#define _POWERPC_HV_REGS_H

#include <regs.h>

#define SPRN_CTRL_R	136
#define SPRN_CTRL_W	152

#define SPRN_HSPRG0	304
#define SPRN_HSPRG1	305
#define SPRN_HDEC	310
#define SPRN_RMOR	312
#define SPRN_HRMOR	313
#define SPRN_HSRR0	314
#define SPRN_HSRR1	315
#define SPRN_LPCR	318
#define SPRN_LPIDR	319

#define SPRN_TSC	921

#define SPRN_TLB_HINT     946
#define SPRN_TLB_INDEX    947
#define SPRN_TLB_VPN      948
#define SPRN_TLB_RPN      949

#ifndef __ASSEMBLY__

#include <types.h>

/* FIXME: this is probably redundant */
union exc_spr {
	uval exc_spr_store[8];
	struct {
		uval srr0;
		uval srr1;
		uval lr;
		uval cr;
		uval ctr;
		uval xer;
	} exc_spr_regs;
};

void ex_trace_handler_spr(uval *r3, union exc_spr *r4);

#ifndef SPRN_HRMOR
/* inline opcodes */
static __inline__ uval
get_hrmor(void)
{
	return 0;
}
#endif

/* inline opcodes */
static __inline__ uval
get_hid0(void)
{
	uval ret;

	/* *INDENT-OFF* */
	__asm__ __volatile__("mfspr %0,%1"
			     :"=r"(ret)
			     :"i" (SPRN_HID0));
	/* *INDENT-ON* */

	return ret;
}

/* inline opcodes */
static __inline__ void
set_hid0(uval val)
{
	/* *INDENT-OFF* */
	__asm__ __volatile__("sync ; mtspr %0, %1;"
			     "mfspr %1, %0;\n\t"
			     "mfspr %1, %0;\n\t"
			     "mfspr %1, %0;\n\t"
			     "mfspr %1, %0;\n\t"
			     "mfspr %1, %0;\n\t"
			     "mfspr %1, %0;\n\t"
			     : /* output */
			     : "i"(SPRN_HID0), "r"(val));
	/* *INDENT-ON* */
}

/* inline opcodes */
static __inline__ uval
get_hid1(void)
{
	uval ret;

	/* *INDENT-OFF* */
	__asm__ __volatile__("mfspr %0,%1"
			     : "=r" (ret)
			     : "i" (SPRN_HID1));
	/* *INDENT-ON* */

	return ret;
}

/* inline opcodes */
static __inline__ void
set_hid1(uval val)
{
	/* *INDENT-OFF* */
	__asm__ __volatile__("mtspr %0, %1; mtspr %0, %1; isync;"
			     : /* output */
			     : "i"(SPRN_HID1), "r"(val));
	/* *INDENT-ON* */
}

/* inline opcodes */
static __inline__ uval
get_hid4(void)
{
	uval ret;

	/* *INDENT-OFF* */
	__asm__ __volatile__("mfspr %0,%1"
			     : "=r" (ret)
			     : "i" (SPRN_HID4));
	/* *INDENT-ON* */

	return ret;
}

static __inline__ void
set_hid4(uval val)
{
	/* *INDENT-OFF* */
	__asm__ __volatile__("sync ; mtspr %0, %1; isync"
			     : /* output */
			     : "i"(SPRN_HID4), "r"(val));

	/* *INDENT-ON* */
}

static __inline__ uval
get_hid5(void)
{
	uval ret;

	/* *INDENT-OFF* */
	__asm__ __volatile__("mfspr %0,%1"
			     : "=r" (ret)
			     : "i" (SPRN_HID5));
	/* *INDENT-ON* */

	return ret;
}

static __inline__ void
set_hid5(uval val)
{
	/* *INDENT-OFF* */
	__asm__ __volatile__("sync ; mtspr %0, %1; isync"
			     : /* output */
			     : "i"(SPRN_HID5), "r"(val));
	/* *INDENT-ON* */
}

static __inline__ uval
mfhsrr0(void)
{
	uval ret;

	/* *INDENT-OFF* */
	__asm__ __volatile__("mfspr %0,%1"
			     : "=&r" (ret)
			     : "i" (SPRN_HSRR0));
	/* *INDENT-ON* */

	return ret;
}

static __inline__ uval
mfhsrr1(void)
{
	uval ret;

	/* *INDENT-OFF* */
	__asm__ __volatile__("mfspr %0,%1"
			     : "=&r" (ret)
			     : "i" (SPRN_HSRR1));
	/* *INDENT-ON* */

	return ret;
}

static __inline__ void
mthsrr0(uval val)
{
      __asm__("mtspr %0,%1": /* output */ :"i"(SPRN_HSRR0), "r"(val));
}

static __inline__ void
mthsrr1(uval val)
{
      __asm__("mtspr %0,%1": /* output */ :"i"(SPRN_HSRR1), "r"(val));
}

static __inline__ uval32
mfhdec(void)
{
	uval32 ret;

	/* *INDENT-OFF* */
	__asm__ __volatile__("mfspr %0,%1"
			     : "=&r" (ret)
			     : "i" (SPRN_HDEC));
	/* *INDENT-ON* */

	return ret;
}

static __inline__ uval32
mftsc(void)
{
	uval32 ret;

	/* *INDENT-OFF* */
	__asm__ __volatile__("mfspr %0,%1"
			     : "=&r" (ret)
			     : "i" (SPRN_TSC));
	/* *INDENT-ON* */

	return ret;
}

static __inline__ void
mthdec(uval32 val)
{
      __asm__("mtspr %0,%1": /* output */ :"i"(SPRN_HDEC), "r"(val));
}

static __inline__ void
mttsc(uval32 val)
{
      __asm__("mtspr %0,%1": /* output */ :"i"(SPRN_TSC), "r"(val));
}

static __inline__ void
mttlb_index(uval val)
{
	/* *INDENT-OFF* */
	__asm__ __volatile__("mtspr %0,%1"
			     : /* output */
			     :"i"(SPRN_TLB_INDEX), "r"(val));
	/* *INDENT-ON* */
}

static __inline__ uval
mftlb_index(void)
{
	uval ret;

	/* *INDENT-OFF* */
	__asm__ __volatile__("mfspr %0,%1"
			     : "=&r" (ret)
			     : "i" (SPRN_TLB_INDEX));
	/* *INDENT-ON* */

	return ret;
}

static __inline__ void
mttlb_rpn(uval val)
{
	/* *INDENT-OFF* */
	__asm__ __volatile__("mtspr %0,%1"
			     : /* output */
			     : "i" (SPRN_TLB_RPN), "r" (val));
	/* *INDENT-ON* */
}

static __inline__ void
mttlb_vpn(uval val)
{
	/* *INDENT-OFF* */
	__asm__ __volatile__("mtspr %0,%1"
			     : /* output */
			     : "i" (SPRN_TLB_VPN), "r" (val));
	/* *INDENT-ON* */
}

static __inline__ void
mthsprg0(uval val)
{
	/* *INDENT-OFF* */
	__asm__ __volatile__("mtspr %0,%1"
			     : /* output */
			     : "i" (SPRN_HSPRG0), "r" (val));
	/* *INDENT-ON* */
}

static __inline__ uval
mfhsprg0(void)
{
	uval hsprg0;

	/* *INDENT-OFF* */
	__asm__ __volatile__("mfspr %0,%1"
			     : "=&r"(hsprg0)
			     : "i" (SPRN_HSPRG0));
	/* *INDENT-ON* */

	return hsprg0;
}

static __inline__ void
mthsprg1(uval val)
{
	/* *INDENT-OFF* */
	__asm__ __volatile__("mtspr %0,%1"
			     : /* output */
			     : "i" (SPRN_HSPRG1), "r" (val));
	/* *INDENT-ON* */
}

static __inline__ uval
mfhsprg1(void)
{
	uval hsprg1;

	/* *INDENT-OFF* */
	__asm__ __volatile__("mfspr %0,%1"
			     : "=&r"(hsprg1)
			     : "i" (SPRN_HSPRG1));
	/* *INDENT-ON* */

	return hsprg1;
}

static __inline__ void
mtr13(uval val)
{
	/* *INDENT-OFF* */
	__asm__ __volatile__("mr 13,%1"
			     : /* output */
			     : "i" (13), "r" (val));
	/* *INDENT-ON* */
}

static __inline__ uval
mfr13(void)
{
	uval tca;

	/* *INDENT-OFF* */
	__asm__ __volatile__("mr %0, 13"
			     : "=r" (tca));
	/* *INDENT-ON* */

	return tca;
}

static __inline__ void
mtsdr1(uval val)
{
	/* *INDENT-OFF* */
	__asm__ __volatile__("mtsdr1 %0"
			     : /* output */
			     : "r" (val));
	/* *INDENT-ON* */
}

static __inline__ uval32
mfctrl(void)
{
	uval32 ret;

	/* *INDENT-OFF* */
	__asm__ __volatile__("mfspr %0,%1"
			     : "=&r" (ret)
			     : "i" (SPRN_CTRL_R));
	/* *INDENT-ON* */

	return ret;
}

static __inline__ void
mtctrl(uval32 val)
{
	/* *INDENT-OFF* */
	__asm__ __volatile__("mtspr %0,%1"
			     : /* output */
			     : "i" (SPRN_CTRL_W), "r" (val));
	/* *INDENT-ON* */
}

static __inline__ void
mtlpcr(uval val)
{

	/* *INDENT-OFF* */
	__asm__ __volatile__("mtspr %0,%1"
			     : /* output */ :
			     "i" (SPRN_LPCR), "r" (val));
	/* *INDENT-ON* */
}

struct thread_control_area;

static __inline__ struct thread_control_area *
get_tca(void)
{
	return (struct thread_control_area *)mfr13();
}

static __inline__ uval32
get_proc_id(void)
{
	return (mfpir() >> 1);
}

static __inline__ uval32
get_thread_id(void)
{
	return mfpir() & 0x1;
}

static inline uval32
get_tca_index(void)
{
	uval32 proc = get_proc_id();
	uval32 thread = get_thread_id();

	return ((proc * THREADS_PER_CPU) + thread);
}

#endif /* ! __ASSEMBLY__ */

#endif /* _POWERPC_HV_REGS_H */
