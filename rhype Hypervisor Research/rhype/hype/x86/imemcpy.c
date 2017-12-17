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
 *
 * Copy data back and forth between hypervisor and partitions.
 *
 */

#include <config.h>
#include <lib.h>
#include <os.h>
#include <hype.h>
#include <mmu.h>
#include <pmm.h>
#include <vm.h>
#include <util.h>
#include <debug.h>
#include <h_proto.h>

/* the size of the temp buffer used for copy in/out operations */
#define	COPY_BUFSIZE	(16 * PGSIZE)

static uval copybuf1 = INVALID_LOGICAL_ADDRESS;
static uval copybuf2 = INVALID_LOGICAL_ADDRESS;

uval
imemcpy_init(uval eomem)
{
	copybuf1 = get_pages(&phys_pa, COPY_BUFSIZE);
	copybuf2 = get_pages(&phys_pa, COPY_BUFSIZE);
	assert(copybuf1 != PAGE_ALLOC_ERROR && copybuf2 != PAGE_ALLOC_ERROR,
	       "No room for inter-partition copy buffers");
	return eomem;		/* XXX this interface should change */
}

/*
 * Copy out to an unmapped source
 */
void *
copy_out(void *dest, const void *src, uval n)
{
	uval cd = (uval)dest;
	uval cs = (uval)src;
	uval misaligned_bytes;
	uval nbytes;

#ifdef COPYOUT_DEBUG
	hprintf("copy_out: dest: %p, src: %p, size: 0x%lx\n", dest, src, n);
#endif

	assert(copybuf1 != INVALID_LOGICAL_ADDRESS, "bad copybuf1");

	misaligned_bytes = cd - ALIGN_DOWN(cd, PGSIZE);

	while (n > 0) {
		nbytes = MIN(COPY_BUFSIZE - misaligned_bytes, n);

		hv_map(copybuf1, cd - misaligned_bytes,
		       ALIGN_UP(nbytes, PGSIZE), PTE_RW, 1);
		memcpy((void *)(copybuf1 + misaligned_bytes),
		       (void *)cs, nbytes);

		n -= nbytes;
		cs += nbytes;
		cd += nbytes;
		misaligned_bytes = 0;
	}

	return dest;
}

/*
 * Copy in from an unmapped source
 */
void *
copy_in(void *dest, const void *src, uval n)
{
	uval cd = (uval)dest;
	uval cs = (uval)src;
	uval misaligned_bytes;

#ifdef COPYIN_DEBUG
	hprintf("copy_in: dest: %p, src: %p, size: 0x%lx\n", dest, src, n);
#endif

	assert(copybuf2 != INVALID_LOGICAL_ADDRESS, "bad copybuf2");

	misaligned_bytes = cs - ALIGN_DOWN(cs, PGSIZE);

	while (n > 0) {
		uval nbytes = MIN(COPY_BUFSIZE - misaligned_bytes, n);
		uval map_size = ALIGN_UP(nbytes + misaligned_bytes, PGSIZE);

		hv_map(copybuf2, cs - misaligned_bytes, map_size, PTE_RW, 1);
		memcpy((void *)cd,
		       (void *)(copybuf2 + misaligned_bytes), nbytes);

		n -= nbytes;
		cs += nbytes;
		cd += nbytes;
		misaligned_bytes = 0;
	}

	return dest;
}

/*
 * Zero physical memory at dest
 */
void *
zero_mem(void *dest, uval n)
{
	uval cd = (uval)dest;
	uval misaligned_bytes;
	uval nbytes;

#ifdef COPYOUT_DEBUG
	hprintf("zero_mem: dest: %p, size: %d\n", dest, n);
#endif

	assert(copybuf1 != INVALID_LOGICAL_ADDRESS, "bad copybuf1");

	misaligned_bytes = cd - ALIGN_DOWN(cd, PGSIZE);

	while (n > 0) {
		nbytes = MIN(COPY_BUFSIZE - misaligned_bytes, n);

		hv_map(copybuf1, cd - misaligned_bytes,
		       ALIGN_UP(nbytes, PGSIZE), PTE_RW, 1);
		memset((void *)(copybuf1 + misaligned_bytes), 0, nbytes);

		n -= nbytes;
		cd += nbytes;
		misaligned_bytes = 0;
	}

	return dest;
}

/*
 * Copy physical memory from src to dest
 */
void *
copy_mem(void *dest, const void *src, uval n)
{
	uval size;
	uval misaligned_src;
	uval misaligned_dest;
	uval cd = (uval)dest;
	uval cs = (uval)src;

	assert(copybuf1 != INVALID_LOGICAL_ADDRESS, "bad copybuf1");
	assert(copybuf2 != INVALID_LOGICAL_ADDRESS, "bad copybuf2");

	while (n > 0) {
		misaligned_src = cs - ALIGN_DOWN(cs, PGSIZE);
		misaligned_dest = cd - ALIGN_DOWN(cd, PGSIZE);

		size = MIN(COPY_BUFSIZE - misaligned_src, n);
		size = MIN(COPY_BUFSIZE - misaligned_dest, size);

		hv_map(copybuf1, cs - misaligned_src,
		       ALIGN_UP(size, PGSIZE), PTE_RW, 1);
		hv_map(copybuf2, cd - misaligned_dest,
		       ALIGN_UP(size, PGSIZE), PTE_RW, 1);

		memcpy((void *)(copybuf2 + misaligned_dest),
		       (void *)(copybuf1 + misaligned_src), size);

		cd += size;
		cs += size;
		n -= size;
	}

	return dest;
}
