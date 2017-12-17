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

extern char _of_image32_start[];
extern char _of_image32_end[];

#define CHUNK_SIZE        (1UL << LOG_CHUNKSIZE)

uval
prom_load(struct partition_status* ps, uval ofd)
{
	uval va_start = 0;
#ifdef USE_OPENFIRMWARE
	struct load_file_entry lf;
	struct loader_segment_entry *le;
	uval rc = 0;
	int elftype;
	uval file_start;
	uval file_size;
	uval va_entry;
	uval va_tocp;
	uval image_start;
	uval image_end;
	uval size;
	uval pr_start;
	uval sz;

	image_start = (uval)_of_image32_start;
	image_end = (uval)_of_image32_end;

	size = image_end - image_start;

	elftype = tryElf(&lf, (uval)image_start);
	if (elftype != KK_64 && elftype != KK_32) {
		/* no image present */
		return 0;
	}


	/* Get Elf data */
	le = &(lf.loaderSegmentTable[0]);
	assert(le->file_size < size,
	       "elf file size [0x%llx] >= image size [0x%lx].\n",
	       le->file_size, size);
	file_start = le->file_start;
	file_size = le->file_size;
	va_start = le->va_start;
	pr_start = le->mem_size;
	va_entry = lf.va_entry;
	va_tocp = lf.va_tocPtr;


	sz = ofd_space(ofd);

	if (va_start == 0) {
		/* need to figure out where this does not collide with
		 * the actual os */
#if LINUX_DONT_LIKE_THIS
		va_start =
			ALIGN_DOWN(CHUNK_SIZE - (pr_start + sz + PGSIZE),
				   PGSIZE);
#else
		va_start = ALIGN_DOWN(CHUNK_SIZE - (1024 * 1024), PGSIZE);
#endif
	}

	/* we need at least 8 byte alignment, but this can't hurt */
	pr_start = ALIGN_UP(pr_start + va_start, PGSIZE);

	*((uval32 *)((uval)file_start + 0x10)) = pr_start;
	hprintf("insert prom image @: 0x%lx, 0x%lx\n", va_start, file_size);

	rc = load_in_lpar(ps, va_start, file_start, file_size);
	assert(rc, "loading prom in lpar memory failed\n");

	hprintf("insert prop image @: 0x%lx, 0x%lx\n", pr_start, sz);

	rc = load_in_lpar(ps, pr_start, ofd, sz);
	assert(rc, "loading prom tree in lpar memory failed\n");
#else
	tree = tree;
	sz = sz;
#endif

	return va_start;
}
