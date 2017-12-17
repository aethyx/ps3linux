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
 * set the value of a debug register
 */

#include <config.h>
#include <pmm.h>
#include <cpu_thread.h>
#include <vm.h>
#include <h_proto.h>
#include <hcall.h>

/* get/set the debug registers
 *
 * If selector is 0, h_dr returns the register values.
 * Otherwise bits 0-7 indicate that dr0-dr7 are to be set.
 * 
 * It is invalid to set dr4 or dr5 (architecturally reserved)
 */
sval
h_dr(struct cpu_thread *thread, uval selector,
     uval dr0, uval dr1, uval dr2, uval dr3,
     uval dr4 __attribute__ ((unused)),
     uval dr5 __attribute__ ((unused)), uval dr6, uval dr7)
{
	uval i;

	/* get register values */
	if (selector == 0) {
		for (i = 0; i < 8; i++)
			return_arg(thread, i + 1, thread->tss.drs[i]);
		return H_Success;
	}

	/* do all checks before sets for atomicity */
	for (i = 0; i < 4; i++)
		if ((selector & 1 << i) && (&dr0)[i] >= HV_VBASE)
			return H_Parameter;

	/*  dr6: bits 4-11, 16-31 must be 1
	 *       bit 12 must be 0
	 */
	dr6 = (dr6 & ~(1 << 12)) | 0xff << 4 | 0xffff << 16;

	if (selector & H_DR7) {
		uval x;

		/* bit 10 must be 1
		 * bits 11-12, 14-15, GD(13) must be 0
		 */
		dr7 = (dr7 & ~(3 << 11 | 7 << 13)) | 0x400;

		/* r/w0-r/w3, len0-len3 must not be 10 */
		for (x = 16; x < 32; x += 2) {
			if (((dr7 >> x) & 3) == 2) {
				return H_INVAL;
			}
		}
	}

	if (selector & (H_DR4 | H_DR5))
		return H_INVAL;

#define set_dr(n, val)						\
	if (selector & 1 << n) {				\
		thread->tss.drs[n] = val;			\
		__asm__("movl %0, %%db" # n :: "r" (val));	\
	}
	set_dr(0, dr0);
	set_dr(1, dr1);
	set_dr(2, dr2);
	set_dr(3, dr3);
	set_dr(6, dr6);
	set_dr(7, dr7);

	return H_Success;
}
