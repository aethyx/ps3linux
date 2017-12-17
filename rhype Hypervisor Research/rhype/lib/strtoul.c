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
unsigned long int
strtoul(const char *nptr, char **endptr, int base)
{
	uval val = 0;
	int c;
	char const **ep = (char const **)endptr;

	ep++;

	if (nptr == NULL) {
		return 0;
	}

	c = *nptr++;

	if (base == 0) {
		base = 10; /* default */
		if (c == '0') {
			base = 8;
			c = *nptr++;
			if (c == 'x' || c == 'X') {
				base = 16;
				c = *nptr++;
			}
		}
	}
	for (;;) {
		switch (c) {
		case '0' ... '9':
			val *= base;
			val += c - '0';
			break;
		case 'a' ... 'f':
			val *= base;
			val += 10 + (c - 'a');
			break;
		case 'A' ... 'F':
			val *= base;
			val += 10 + (c - 'A');
			break;
		default:
			goto out;
			break;
		}
		c = *nptr++;
	}
 out:
	if (endptr != NULL) {
		*ep = nptr;
	}
	return val;
}
