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
 * map the destination, do a memcpy, then unmap from the page tables
 *
 * Generic version.. assumes that HV has Direct access to physical space.
 *
 */

#include <lib.h>
#include <pmm.h>

uval
imemcpy_init(uval ea)
{
	return ea;
}

void *
copy_out(void *dest, const void *src, uval n)
{
	uval i = 0;
	void *p = memcpy(dest, src, n);

	/*
	 *Should we always do this?
	 * Perhaps best to integrate directly into memcpy.
	 */
	while (i < n) {
		icbi(((uval)p) + i);
		sync();
		isync();
		i += CACHE_LINE_SIZE;
	}
	return p;
}

void *
copy_in(void *dest, const void *src, uval n)
{
	return memcpy(dest, src, n);
}

void *
zero_mem(void *dest, uval n)
{
	return memset(dest, 0, n);
}

/*
 * Copy physical memory from src to dest
 */
void *
copy_mem(void *dest, const void *src, uval n)
{
	return memcpy(dest, src, n);
}
