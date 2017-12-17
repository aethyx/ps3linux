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
 * bit operations
 */

#ifndef _32_BITOPS_H
#define _32_BITOPS_H

#include <config.h>
#include <lib.h>

static inline uval
bit_log2(uval x)
{
	__asm__("cntlzw %0, %1": "=r"(x) : "r"(x));
	return 31 - x;
}

#include_next <bitops.h>

#endif /* ! _32_BITOPS_H */
