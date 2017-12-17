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
 * eXternal Interrupt Routing Register
 */

#include <config.h>
#include <xirr.h>
#include <h_proto.h>
#include <xir.h>
#include <hv_regs.h>
#include <debug.h>
sval
h_xirr(struct cpu_thread *thread)
{
	struct xir_queue *xq = &thread->xirq;
	uval xirr;

	if (xirq_empty(xq)) {
		xirr = 0;
	} else {
		uval class;

 		xirr = xirq_pop(xq);
		class = xirr_class(xirr);
		if (class == XIRR_CLASS_HWDEV) {
			sval rc;

			/* EOI is from here, IOHost will EOI itself
			 * and never call H_EOI */
			rc = xirr_classes[class].eoi_fn(thread, xirr);
			assert(rc == H_Success, "auto eoi failed\n");
		}
	}

	return_arg(thread, 1, xirr);
	return H_Success;
}
