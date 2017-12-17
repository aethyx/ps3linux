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

#include <types.h>
#include <lib.h>

sval
char2hex(char c)
{
	if ((c >= 'a') && (c <= 'f'))
		return c - 'a' + 10;
	
	if ((c >= '0') && (c <= '9'))
		return c - '0';
	
	if ((c >= 'A') && (c <= 'F'))
		return c - 'A' + 10;
	
	return -1;
}


char
hex2char(uval x)
{
	const char array[] = "0123456789abcdef";

	return array[x & 15];
}


char
str2hex(const char *str)
{
	return (char2hex(str[0]) << 4) | char2hex(str[1]);
}


uval
str2uval(const char *str, uval bytes)
{
	uval x = 0;
	uval i = 0;

	while (*str && i < (bytes * 2)) {
		x <<= 4;
		x += char2hex(*str);
		++str;
		++i;
	}
	
	return x;
}
