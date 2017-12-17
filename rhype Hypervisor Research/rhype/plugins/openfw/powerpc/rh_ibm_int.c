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

#include <config.h>
#include <rh.h>
#include <hcall.h>
#include <lpar.h>

sval
rh_ibm_int_on(uval nargs, uval nrets,
	      uval argp[], uval retp[], uval ba __attribute__ ((unused)))
{
	if (nargs == 1) {
		if (nrets == 1) {
			uval32 intr = argp[0];
			sval rc;

			rc = hcall_interrupt(NULL, intr, 1);
			if (rc == H_Success) {
				retp[0] = RTAS_SUCCEED;
				return RTAS_SUCCEED;
			}
		}
	}
	retp[0] = RTAS_PARAM_ERROR;
	return RTAS_PARAM_ERROR;
}

sval
rh_ibm_int_off(uval nargs, uval nrets,
	       uval argp[], uval retp[], uval ba __attribute__ ((unused)))
{
	if (nargs == 1) {
		if (nrets == 1) {
			uval32 intr = argp[0];
			sval rc;

			rc = hcall_interrupt(NULL, intr, 0);
			if (rc == H_Success) {
				retp[0] = RTAS_SUCCEED;
				return RTAS_SUCCEED;
			}
			return RTAS_SUCCEED;
		}
	}
	retp[0] = RTAS_PARAM_ERROR;
	return RTAS_PARAM_ERROR;
}
