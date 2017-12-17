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
#include <hype.h>
#include <os.h>
#include <cpu_thread.h>
#include <types.h>
#include <of_devtree.h>
#include <eic.h>
#include <io.h>
#include <debug.h>

typedef volatile uval32 opreg_t;

union op_feature_reg {
	uval32 word;
	struct {
		/* *INDENT-OFF* */
		uval32	reserved_31_27	:5;
		uval32	num_IRQ		:11;
		uval32	reserved_16_13	:3;
		uval32	num_cpu		:5;
		uval32	version		:8;
		/* *INDENT-ON* */
	} bits;
};

union op_global_config_reg {
	uval32 word;
	struct {
		/* *INDENT-OFF* */
		uval32	reset		:1;
		uval32	reserved_30	:1;
		uval32	pass_through	:1;
		uval32	reserved_29_20	:9;
		uval32	base		:20;
		/* *INDENT-ON* */
	} bits;
};

union op_vendor_id_reg {
	uval32 word;
	struct {
		/* *INDENT-OFF* */
		uval32	reserved_31_24	:8;
		uval32	stepping	:8;
		uval32	device_id	:8;
		uval32	vendor_id	:8;
		/* *INDENT-ON* */
	} bits;
};

union op_int_ack {
	uval32 word;
	struct {
		/* *INDENT-OFF* */
		uval32	reserved_31_16	:16;
		uval32	reserved_16_11	:6;
		uval32	vector		:10;
		/* *INDENT-ON* */
	} bits;
};

static uval op_endian = 0;	/* 0 - BE, 1 - LE */

static inline uval32
read_reg(const uval32 volatile * ptr)
{
	if (op_endian == 1) {
		return io_in32LE(ptr);
	} else {
		return io_in32(ptr);
	}
}

static inline void
write_reg(uval32 volatile * ptr, uval32 val)
{
	if (op_endian == 1) {
		io_out32LE(ptr, val);
	} else {
		io_out32(ptr, val);
	}
}

struct openpic_global {
	const opreg_t features;
	const uval8 __pad1[0x1c];

	opreg_t config;
	const uval8 __pad2[0xc];

	const uval8 __vendor_specific[0x80 - 0x30];

	opreg_t vendor_id;
	const uval8 __pad3[0xc];

	opreg_t processor_init;
	const uval8 __pad4[0xc];

	opreg_t ipi0_privec;
	const uval8 __pad5[0xc];

	opreg_t ipi1_privec;
	const uval8 __pad6[0xc];

	opreg_t ipi2_privec;
	const uval8 __pad7[0xc];

	opreg_t ipi3_privec;
	const uval8 __pad8[0xc];

	opreg_t spurious;
	const uval8 __pad9[0xc];
};

struct openpic_cpu {
	const uval8 __pad1[0x40];
	struct {
		opreg_t reg;
		const opreg_t __pad[3];
	} ipi_dispatch[4];

	opreg_t task_priority;
	const uval8 __pad2[0xc];

	opreg_t who_am_i;
	const uval8 __pad3[0xc];

	opreg_t intr_ack;
	const uval8 __pad4[0xc];

	opreg_t eoi;
	const uval8 __pad5[0xc];

	const uval8 __pad6[0x1000 - 0xc0];
};

struct openpic {
	struct openpic_cpu local;
	struct openpic_global global;
	const uval8 __pad1[0x20000 -
			   sizeof (struct openpic_global) -
			   sizeof (struct openpic_cpu)];
	struct openpic_cpu cpu_specific[0x20];
};

static struct openpic *openpic = (typeof(openpic)) 0xffc00000;

static int
openpic_intr_handler(xirr_t xirr, struct xh_data *xh, uval payload,
		     struct cpu_thread **receiver)
{
	(void)payload;
	int pos;
	struct cpu_thread *thread = eic_default_thread();

	assert(thread != NULL, "destination thread not set\n");

	pos = __xir_enqueue(xirr, xh, thread);

	if (receiver) {
		*receiver = thread;
	}

	return pos;
}

static sval
openpic_eoi(struct cpu_thread *thr, xirr_t xirr)
{
	xir_eoi(thr, xirr);
	return H_Success;
}

static sval
openpic_config(uval arg1, uval arg2, uval arg3, uval arg4)
{
	(void)arg3;
	(void)arg4;

	xir_init_class(XIRR_CLASS_HWDEV, openpic_intr_handler, NULL);
	xirr_classes[XIRR_CLASS_HWDEV].eoi_fn = openpic_eoi;

	openpic = (typeof(openpic)) arg1;
	op_endian = arg2;
	return 0;
}

extern sval eic_config(uval arg1, uval arg2, uval arg3, uval arg4)
	__attribute__ ((alias("openpic_config")));

static sval
openpic_set_line(uval lpid, uval cpu, uval thr, uval hw_src, uval *isrc)
{
	xirr_t xirr;
	struct cpu_thread *t;
	struct os *os = os_lookup(lpid);

	assert(hw_src < XIRR_DEVID_SZ, "isrc too big\n");

	if (!os) {
		return H_Parameter;
	}

	if (cpu || thr) {
		hprintf("OpenPIC doesn't support cpu/thread routing "
			"(isrc %lx -> %lx:%lx:%lx\n", hw_src, lpid, cpu, thr);
	}

	t = &os->cpu[cpu]->thread[thr];
	xirr = xirr_encode(hw_src, XIRR_CLASS_HWDEV);

	struct xh_data *xh = xir_get_xh_data(xirr);

	__xirr_lock(xh);
	xh->xh_data = NULL;
	xh->xh_flags &= ~XIRR_DATA_THR;
	__xirr_unlock(xh);

	xir_default_config(xirr, t, NULL);

	*isrc = xirr;
	return 0;
}

extern sval
     eic_set_line(uval lpid, uval cpu, uval thr, uval hw_src, uval *isrc)
	__attribute__ ((alias("openpic_set_line")));

/*
 * this checks the exceptions from the OpenPIC controler
 */
static struct cpu_thread *
__openpic_do_exception(struct cpu_thread *thread)
{
	int count = 0;
	struct cpu_thread *curr = thread;
	union op_int_ack op_vec;


	/* assume there is no thread for the interrupt */
	thread = NULL;

	xirr_t xirr;
	int h;

	op_vec.word = read_reg(&openpic->cpu_specific[0].intr_ack);
	xirr = xirr_encode(op_vec.bits.vector, XIRR_CLASS_HWDEV);

	++count;
	h = xir_raise(xirr, &thread);

	/*write_reg(&openpic->cpu_specific[0].eoi, 0);*/

	if (h <= 0) {		/* Interrupt not raised */
		return curr;
	}

	/* Interrupt raised */
	if (thread && (is_colocated(thread, curr) != THREAD_MATCH)) {
		thread = curr;
	}

	return thread;
}

extern struct cpu_thread *eic_exception(struct cpu_thread *thread)
	__attribute__ ((alias("__openpic_do_exception")));
