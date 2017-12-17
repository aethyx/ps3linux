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
#include <test/sysipc.h>

sval
ofh_boot(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	b=b;
	nargs = nargs;
	nrets = nrets;
	argp = argp;
	retp = retp;
	return OF_FAILURE;
}

sval
ofh_enter(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	b=b;
	nargs = nargs;
	nrets = nrets;
	argp = argp;
	retp = retp;
	return OF_FAILURE;
}

sval
ofh_exit(uval nargs __attribute__ ((unused)),
	 uval nrets  __attribute__ ((unused)),
	 sval argp[]  __attribute__ ((unused)),
	 sval retp[] __attribute__ ((unused)),
	 uval b __attribute__ ((unused)))
{
	for (;;) {
		hcall_send_async(NULL, CONTROLLER_LPID,
				 IPC_SUICIDE, 0, 0, 0);
		hcall_yield(NULL, 0);
	}
	return OF_FAILURE;
}

sval
ofh_chain(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	b=b;
	nargs = nargs;
	nrets = nrets;
	argp = argp;
	retp = retp;
	return OF_FAILURE;
}

