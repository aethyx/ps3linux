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
#include <h_proto.h>
#include <vtty.h>
#include <vterm.h>
#include <lib.h>

sval
h_put_term_char(struct cpu_thread *thread, uval channel, uval count,
		uval chars1, uval chars2, uval chars3, uval chars4)
{
	uval data[4] = { chars1, chars2, chars3, chars4 };

	if (count > 16) {
		return H_Parameter;
	}
	return vt_char_put(thread, channel, count, (const char *)data);
}
