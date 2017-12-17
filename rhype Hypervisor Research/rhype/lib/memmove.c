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

/* should this use (aligned) words instead of bytes? yes. :) */
void *
memmove(void *dest, const void *src, size_t n)
{
	uval i = n;

	if (dest > src) {
		/* backwards */
		char *cd = ((char *)dest);
		const char *cs = ((const char *)src);

		cd += n - 1;
		cs += n - 1;

		while (i > 0) {
			*cd-- = *cs--;
			i--;
		}
	} else {
		/* forwards */
		char *cd = ((char *)dest);
		const char *cs = ((const char *)src);

		while (i > 0) {
			*cd++ = *cs++;
			i--;
		}
	}

	return dest;
}
