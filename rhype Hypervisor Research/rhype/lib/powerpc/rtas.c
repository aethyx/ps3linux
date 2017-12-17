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
#include <rtas.h>
#include <openfirmware.h>
#include <lib.h>
#include <util.h>
#include <mmu.h>

uval rtas_entry = 0;
static uval rtas_msr;
static uval rtas_base;

uval
rtas_instantiate(uval mem, uval msr)
{
	phandle pkg;
	sval ret;
	sval32 size;
	ihandle rtas;

	rtas_msr = msr;

	pkg = of_finddevice("/rtas");
	if (pkg == OF_FAILURE) return mem;

	ret = of_getprop(pkg, "rtas-size", &size, sizeof (size));
	assert(ret != OF_FAILURE, "of_getprop(rtas-size)");

	/* FIXME: Hack for SLOF.  We want OF to show rtas is present
	 * with size 0.  Linux detects hyperrtas, and uses LPAR
	 * interfaces, but won't use RTAS, which doesn't really exist,
	 * until we implement an RTAS layer.
	 */
	if (size==0)
		return 0;

	ret = call_of("open", 1, 1, &rtas, "/rtas");

	if (ret == OF_SUCCESS) {
		sval32 res[2];

		mem = ALIGN_UP(mem, PGSIZE);

		ret = call_of("call-method", 3, 2, res,
		      "instantiate-rtas", rtas, mem);

		assert(ret == OF_SUCCESS, "call-method(/instantiate-rtas)");
		hprintf("rtas located at 0x%x\n", res[1]);
		rtas_entry = res[1];
		rtas_base = mem;

		return ALIGN_UP(mem + size, PGSIZE);
	}
	return 0;
}

sval32
rtas_call(struct rtas_args_s *r)
{
	return prom_call(r, rtas_base, rtas_entry, rtas_msr);
}
