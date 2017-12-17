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

#include <ofh.h>
#include <lib.h>
#include <hcall.h>
#include <lpar.h>

sval
ofh_start_cpu(uval nargs, uval nrets, sval argp[],
	      sval retp[] __attribute__ ((unused)),
	      uval b __attribute__ ((unused)))
{
	if (nargs == 3) {
		if (nrets == 0) {
			ofdn_t ph = argp[0];
			uval pc = argp[1];
			uval arg = argp[2];
			
			(void)ph; (void)pc; (void)arg;
			return OF_FAILURE;
		}
	}
	return OF_FAILURE;
}

sval
ofh_stop_self(uval nargs, uval nrets,
	      sval argp[] __attribute__ ((unused)),
	      sval retp[] __attribute__ ((unused)),
	      uval b __attribute__ ((unused)))
{
	if (nargs == 0) {
		if (nrets == 0) {
			return OF_FAILURE;
		}
	}
	return OF_FAILURE;
}

sval
ofh_idle_self(uval nargs, uval nrets,
	      sval argp[] __attribute__ ((unused)),
	      sval retp[] __attribute__ ((unused)),
	      uval b __attribute__ ((unused)))
{
	if (nargs == 0) {
		if (nrets == 0) {
			return OF_FAILURE;
		}
	}
	return OF_FAILURE;
}
sval
ofh_resume_cpu(uval nargs, uval nrets, sval argp[],
	       sval retp[] __attribute__ ((unused)),
	      uval b __attribute__ ((unused)))
{
	if (nargs == 1) {
		if (nrets == 0) {
			ofdn_t ph = argp[0];
			
			(void)ph;
			return OF_FAILURE;
		}
	}
	return OF_FAILURE;
}
