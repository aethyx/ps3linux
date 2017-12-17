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
#include <lpar.h>
#include <hype.h>
#include <mmu.h>
#include <pmm.h>
#include <h_proto.h>

/*
 * Some silly test data... who uses this anyway ?
 */
static const uval bs_data[] = {
	0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
	0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf
};

/*
 * control has to be signed number because it indicates success/error
 */
sval
h_hypervisor_data(struct cpu_thread *thread, uval ctrl)
{
	uval r;

	/* no need to check for negative.. it will just be too big :) */
	if (ctrl > (sizeof (bs_data) / sizeof (bs_data[0]))) {
		return H_Parameter;
	}

	r = 4;
	while (ctrl < (sizeof (bs_data) / sizeof (bs_data[0])) && r < 12) {
		thread->reg_gprs[r] = bs_data[ctrl];
		++ctrl;
		++r;
	}

	return ctrl;
}
