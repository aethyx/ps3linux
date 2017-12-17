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
#include <rh.h>
#include <hcall.h>
#include <test/sysipc.h>

sval
rh_power_off(uval nargs, uval nrets,
	     uval argp[] __attribute__ ((unused)),
	     uval retp[] __attribute__ ((unused)),
	     uval ba __attribute__ ((unused)))
{
	if (nargs == 2) {
		if (nrets == 1) {
			for (;;) {
				hcall_send_async(NULL,
						 CONTROLLER_LPID,
						 IPC_SUICIDE, 0, 0, 0);
				hcall_yield(NULL, 0);
			}
		}
	}
	retp[0] = RTAS_PARAM_ERROR;
	return RTAS_PARAM_ERROR;
}
