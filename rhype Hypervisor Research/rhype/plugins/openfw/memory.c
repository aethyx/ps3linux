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
#include <util.h>

extern char _end[];
extern char _start[];

struct of_malloc_s {
	uval ofm_start;
	uval ofm_end;
};
static struct of_malloc_s claimed[64];

static sval
claim(uval b, uval virt, uval size, uval align, uval *baseaddr)
{
	struct of_malloc_s *cp;
	uval i;
	sval e;
	uval end;

	if (align != 0) {
		/* we don't do this now */
		return OF_FAILURE;
	}

	end = virt + size;

	/* you cannot claim OF's own space */
	if (virt >= (uval)_start && end < (uval)_end) {
		return OF_FAILURE;
	}

	cp = DRELA(&claimed[0], b);
	/* don't care about speed at the moment */
	e = -1;
	for (i = 0; i < sizeof (claimed)/sizeof (claimed[0]); i++) {
		if (cp[i].ofm_end == 0) {
			if (e == -1) {
				e = i;
			}
			continue;
		}
		if (virt >= cp[i].ofm_start && virt < cp[i].ofm_end) {
			return OF_FAILURE;
		}
		if (end >= cp[i].ofm_start && end < cp[i].ofm_end) {
			return OF_FAILURE;
		}
	}
	/* e points to the first empty */
	cp[e].ofm_start = virt;
	cp[e].ofm_end = end;
	*baseaddr = virt;
	return OF_SUCCESS;
}
	
			
		


sval
ofh_claim(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	if (nargs == 3) {
		if (nrets == 1) {
			uval virt = argp[0];
			uval size = argp[1];
			uval align = argp[2];
			uval *baseaddr = &retp[0];

			return claim(b, virt, size, align, baseaddr);
		}	
	}
	return OF_FAILURE;
}	

static sval
release(uval b, uval virt, uval size)
{
	struct of_malloc_s *cp;
	uval i;
	uval end;

	end = virt + size;

	/* you cannot release OF's own space */
	if (virt >= (uval)_start && end < (uval)_end) {
		return OF_FAILURE;
	}

	cp = DRELA(&claimed[0], b);
	/* don't care about speed at the moment */
	for (i = 0; i < sizeof (claimed)/sizeof (claimed[0]); i++) {
		if (virt == cp[i].ofm_start && end == cp[i].ofm_end) {
			cp[i].ofm_start = 0;
			cp[i].ofm_end = 0;
			return OF_SUCCESS;
		}
	}
	return OF_FAILURE;
}
	
sval
ofh_release(uval nargs, uval nrets, sval argp[],
	    sval retp[] __attribute__ ((unused)),
	    uval b)
{
	if (nargs == 2) {
		if (nrets == 0) {
			uval virt = argp[0];
			uval size = argp[1];

			return release(b, virt, size);
		}	
	}
	return OF_FAILURE;
}	
