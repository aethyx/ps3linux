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
#include <lib.h>

void *
memset(void *s, int c, size_t n)
{
	uval8 *ss = (uval8 *)s;

	if (n == 0) {
		return s;
	}

	/* yes, I pulled the 2 out of this air */
	if (n >= (2 * sizeof (uval))) {
		uval val = 0;
		uval i;

		/* construct val assignment from c */
		if (c != 0) {
			for (i = 0; i < sizeof (uval); i++) {
				val = (val << 8) | c;
			}
		}

		/* do by character until aligned */
		while (((uval)ss & (sizeof (uval) - 1)) > 0) {
			*ss = c;
			++ss;
			--n;
		}

		/* now do the aligned stores */
		while (n >= sizeof (uval)) {
			*(uval *)ss = val;
			ss += sizeof (uval);
			n -= sizeof (uval);
		}
	}
	/* do that last unaligned bit */
	while (n > 0) {
		*ss = c;
		++ss;
		--n;

	}	       

	return s;
}


