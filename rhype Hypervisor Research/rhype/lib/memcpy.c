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

#include <lib.h>
#include <bitops.h>

void *
memcpy(void *dest, const void *src, size_t n)
{
	uval i;
	uval sz;

	/* lowest common denominator */
	sz = (uval)dest | (uval)src | n;	/* get bits */
	sz = 1UL << (ffs(sz) - 1);		/* find first set */
	if (sz > sizeof (uval)) {		/* max at uval */
		sz = sizeof (uval);
	}

	switch (sz) {
	case sizeof (uval8):
		for (i = 0; i < n ; i++) {
			((uval8 *)dest)[i] = ((const uval8 *)src)[i];
		}
		break;
	case sizeof (uval16):
		n /= sizeof (uval16);
		for (i = 0; i < n ; i++) {
			((uval16 *)dest)[i] = ((const uval16 *)src)[i];
		}
		break;
	case sizeof (uval32):
		n /= sizeof (uval32);
		for (i = 0; i < n ; i++) {
			((uval32 *)dest)[i] = ((const uval32 *)src)[i];
		}
		break;
	case sizeof (uval64):
		n /= sizeof (uval64);
		for (i = 0; i < n ; i++) {
			((uval64 *)dest)[i] = ((const uval64 *)src)[i];
		}
		break;
	}

	return dest;
}
