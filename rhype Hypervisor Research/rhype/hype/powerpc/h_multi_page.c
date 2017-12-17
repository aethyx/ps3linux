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

/* yes, we take three sizes because some machines (embedded) can
 * specific a primary page size that is not 4k
 * s1, s2, and s3 are log2 pagesize, 0 means unchanged.
 */
sval
h_multi_page(struct cpu_thread *thread, uval s1, uval s2, uval s3)
{
	/* FIXME: do nothing right now since the support seems to
	 * be broken */
	thread = thread;
	s1 = s1;
	s2 = s2;
	s3 = s3;
	return (H_Success);
}
