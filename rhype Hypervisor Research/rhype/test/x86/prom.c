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

#include <test.h>
#include <loadFile.h>		/* I hate this file */
#include <openfirmware.h>
#include <ofd.h>
#include <mmu.h>

#define CHUNK_SIZE        (1UL << LOG_CHUNKSIZE)

uval
prom_load(struct partition_status* ps, uval ofd)
{
	uval va_start;
	uval rc;
	uval sz;

	sz = ofd_size(ofd);

	va_start = ALIGN_DOWN(CHUNK_SIZE - (1024 * 1024), PGSIZE);

	hprintf("insert prop image @: 0x%lx, 0x%lx\n", va_start, sz);

	rc = load_in_lpar(ps, va_start, ofd, sz);
	assert(rc, "loading prom tree in lpar memory failed\n");

	return va_start;
}
