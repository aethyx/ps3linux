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
#include <h_proto.h>

static sval
h_nosupp(uval token)
{
	assert(0, "hcall(0x%lx) has been called and is unsupported\n", token);
	return (H_Function);
}

sval
h_confer(struct cpu_thread *thread __attribute__ ((unused)))
{
	return h_nosupp(H_CONFER);
}

sval
h_prod(struct cpu_thread *thread __attribute__ ((unused)))
{
	return h_nosupp(H_PROD);
}

sval
h_get_ppp(struct cpu_thread *thread __attribute__ ((unused)))
{
	return h_nosupp(H_GET_PPP);
}

sval
h_set_ppp(struct cpu_thread *thread __attribute__ ((unused)))
{
	return h_nosupp(H_SET_PPP);
}

sval
h_purr(struct cpu_thread *thread __attribute__ ((unused)))
{
	return h_nosupp(H_PURR);
}

sval
h_pic(struct cpu_thread *thread __attribute__ ((unused)))
{
	return h_nosupp(H_PIC);
}

sval
h_putrtce(struct cpu_thread *thread __attribute__ ((unused)))
{
	return h_nosupp(H_PUTRTCE);
}

sval
h_bulk_remove(struct cpu_thread *thread __attribute__ ((unused)))
{
	return h_nosupp(H_BULK_REMOVE);
}

sval
h_write_rdma(struct cpu_thread *thread __attribute__ ((unused)))
{
	return h_nosupp(H_WRITE_RDMA);
}

sval
h_read_rdma(struct cpu_thread *thread __attribute__ ((unused)))
{
	return h_nosupp(H_READ_RDMA);
}

sval
h_set_xdabr(struct cpu_thread *thread __attribute__ ((unused)))
{
	return h_nosupp(H_SET_XDABR);
}

sval
h_put_tce_indirect(struct cpu_thread *thread __attribute__ ((unused)))
{
	return h_nosupp(H_PUT_TCE_INDIRECT);
}

sval
h_put_rtce_inderect(struct cpu_thread *thread __attribute__ ((unused)))
{
	return h_nosupp(H_PUT_RTCE_INDERECT);
}

sval
h_mass_map_tce(struct cpu_thread *thread __attribute__ ((unused)))
{
	return h_nosupp(H_MASS_MAP_TCE);
}

sval
h_alrdma(struct cpu_thread *thread __attribute__ ((unused)))
{
	return h_nosupp(H_ALRDMA);
}

sval
h_return_logical(struct cpu_thread *thread __attribute__ ((unused)))
{
	return h_nosupp(H_RETURN_LOGICAL);
}

sval
h_hca_resv(struct cpu_thread *thread __attribute__ ((unused)))
{
	return h_nosupp(H_HCA_RESV_BEGIN);
}
