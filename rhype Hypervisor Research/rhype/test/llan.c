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
 * A console driver for test OSs.
 *
 */

#include <test.h>
#include <hcall.h>

uval
llan_create(uval dma_size, uval lpid, uval *liobn)
{
	sval rc;
	uval dma_sz = 0;
	uval l;
	uval rets[4];

	rc = hcall_vio_ctl(rets, HVIO_ACQUIRE, HVIO_LLAN, dma_size);
	assert(rc == H_Success, "could not allocate llan\n");

	l = rets[0];
	if (rc == H_Success) {
		dma_sz = rets[2];

		if (lpid != H_SELF_LPID) {
			/* need to transfer this resource here */
			rc = hcall_resource_transfer(rets, INTR_SRC,
						     l, 0, 0, lpid);
			if (rc == H_Success) {
				*liobn = rets[0];
			} else {
				assert(0, "could not transfer llan\n");
				dma_sz = 0;
			}
		} else {
			*liobn = l;
		}
		if (dma_sz == 0) {
			rc = hcall_vio_ctl(NULL, HVIO_RELEASE, l, 0);
			assert(rc == H_Success, "could not release llan\n");
		}
	}
	return dma_sz;
}
	
