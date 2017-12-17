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
#include <mmu.h>
#include <vregs.h>
extern char _gdb_image64_start[];
extern char _gdb_image64_end[];

extern void gdb_stub_load(uval laddr, uval size);

void
gdb_stub_load(uval laddr, uval size)
{
	uval va_start = 0;
	struct load_file_entry lf;
	struct loader_segment_entry *le;
	int elftype;
	uval file_start;
	uval file_size;
	uval va_entry;
	uval va_tocp;
	uval image_start;
	uval image_end;
	uval img_size;
	uval pr_start;

	image_start = (uval)_gdb_image64_start;
	image_end = (uval)_gdb_image64_end;

	img_size = image_end - image_start;

	elftype = tryElf(&lf, (uval)image_start);
	if (elftype != KK_64 && elftype != KK_32) {
		/* no image present */
		return ;
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

	hcall_create_vm_class(NULL, H_VM_CLASS_LINEAR, NUM_KERNEL_VMC - 1,
			      va_start, size, laddr, 0, 0, 0);


	/* we need at least 8 byte alignment, but this can't hurt */
	hprintf("insert gdb image @: 0x%lx, 0x%lx\n", va_start, file_size);

	memset((void*)(va_start + file_size), 0, size - file_size);
	
	memcpy((void*)va_start, (void*)file_start, file_size);
	uval start = ALIGN_DOWN(va_start, PGSIZE);
	while(start < va_start + file_size) {
		icache_invalidate_page(start >> LOG_PGSIZE, LOG_PGSIZE);
		start += PGSIZE;
	}
	
	uval64 func_desc[2] = { lf.va_entry, lf.va_tocPtr };
	
	void (*init)(void) = (typeof(init))func_desc;
	
	init();
	
}
