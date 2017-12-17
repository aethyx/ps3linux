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
 *
 */
#include <ofd.h>

uval
ofd_devtree_init(uval mem __attribute__ (( unused )), uval *space)
{
	*space = 0;
	return 0UL;
}

uval
ofd_lpar_create(struct partition_status *ps __attribute__ (( unused )),
		uval new __attribute__ (( unused )),
		uval mem __attribute__ (( unused )))
{
	return 0;
}


void
ofd_bootargs(uval ofd __attribute__ (( unused )),
	     char *buf __attribute__ (( unused )),
	     uval sz __attribute__ (( unused )))
{
	return;
}
