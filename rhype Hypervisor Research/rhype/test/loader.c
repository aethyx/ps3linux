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

#include <test.h>
#include <loadFile.h> /* I hate this file */
#include <partition.h>
#include <sysipc.h>
#include <xh.h>
#include <mmu.h>
#include <ofd.h>
#include <pgalloc.h>
#include <objalloc.h>
#include <util.h>
#define THIS_CPU 0xffff

#define CHUNK_SIZE (1ULL << LOG_CHUNKSIZE)

/* It a kind of magic */
typedef void (image_t)(void);
typedef void (*imageptr_t)(void);

/* these are magic and are defined by the linker */
extern imageptr_t image_start[];
extern imageptr_t image_size[];
extern char *image_names[];
extern uval image_cnt;

uval curslots;

#define MAX_CHUNKS 32UL

extern struct partition_status partitions[];


uval
load_in_lpar(struct partition_status* ps, uval offset,
	     uval istart, uval isize)
{
	uval ra;
	uval ea;
	uval size;
	ra = ALIGN_DOWN(ps->init_mem + offset, 1 << LOG_CHUNKSIZE);
	size = ALIGN_UP(ps->init_mem + offset - ra + isize, PGSIZE);

	ea = ra;

	map_pages(ra, ea, 1 << LOG_CHUNKSIZE);

	ea += offset;

	hprintf("Copying partition img: 0x%lx[0x%lx] -> 0x%lx\n",
		istart, isize, ea);

	memcpy((void *)ea, (void *)istart, isize);

	isize = ALIGN_UP(isize, PGSIZE);
	while (isize) {
		icache_invalidate_page(ea >> LOG_PGSIZE, LOG_PGSIZE);
		isize -= PGSIZE;
	}

	return 1;
}

static uval
vty_setup(struct partition_status *ps)
{
#ifdef USE_VTERM_JX
	sval rc;
	uval vty = 0;

	rc = hcall_register_vterm(NULL, vty, ps->lpid, 0);
	assert(rc == H_Success, "could not register vterm: 0x%lx\n", vty);

	return 1;
#else
	return ps == ps;
#endif
}

static uval
launch_partition(struct partition_status *ps, struct load_file_entry* lf,
		 uval ofd)
{
	uval ret[1];
	uval rc;
	const uval defslot = 0x1;
	uval schedvals[3];
	sval rot;

	uval magic = lf->loaderSegmentTable[0].file_start;
	magic += 0x10;

	if (*(uval64*)magic == HYPE_PARTITION_INFO_MAGIC_NUMBER) {
		magic = *((uval*)(magic + 8));
	} else {
		magic = INVALID_LOGICAL_ADDRESS;
	}

	hputs("Creating partition\n");
	rc = hcall_create_partition(ret, ps->init_mem, ps->init_mem_size,
				    magic);
	assert(rc == H_Success,
	       "hcall_create_partition() failed rc= 0x%lx\n", rc);

	hprintf("Created partition  ID %ld\n"
		"  starting it @ pc = 0x%llx.\n",
		ret[0], lf->va_entry);
	ps->lpid = ret[0];

	vty_setup(ps);

	rot = hcall_set_sched_params(schedvals, ps->lpid,
				     THIS_CPU, defslot, 0);
	assert(rot >= 0, "set_sched failed\n");
	hprintf("Scheduled 0x%lx: rot: %ld (0x%016lx, 0x%016lx 0x%016lx)\n",
		ps->lpid, rot, schedvals[0], schedvals[1], schedvals[2]);


	curslots = schedvals[0];
	ps->slot = schedvals[2];

	register_partition(ps, ofd);

	if (!start_partition(ps, lf, ofd)) {
		return 0;
	}

	hprintf("%s: yielding to sysId: 0x%lx now\n", __func__, ps->lpid);
	hcall_yield(NULL, ps->lpid);
	hprintf("%s: back from yielding to sysId: 0x%lx\n",
		__func__, ps->lpid);


	return ps->lpid;
}

static void
load_image(struct partition_status* ps,
	   struct load_file_entry* lf, uval load_offset)
{
	struct loader_segment_entry *le;
	int segno = lf->numberOfLoaderSegments;
	uval istart;
	uval isize;
	uval offset;
	int i;

	for (i = 0; i < segno; i++) {
		le = &lf->loaderSegmentTable[i];
		istart = le->file_start;
		isize = le->file_size;
		offset = le->va_start;
		if (le->file_size > 0) {
			hprintf("%s: copying bits from %s: to 0x%lx[0x%lx]\n",
				__func__, ps->name,
				ps->init_mem + offset + load_offset,
				isize);

			load_in_lpar(ps, offset + load_offset, istart, isize);
		}
	}
	lf->va_entry += load_offset;
	lf->va_tocPtr+= load_offset;
}


uval
launch_image(uval init_mem, uval init_mem_size, const char* name, uval ofd)
{
	struct load_file_entry lf;
	uval start = 0;
	uval size = 0;
	uval rc = 0;
	int elftype;
	struct partition_status *ps;
	int i;

	for (i = 0; i < MAX_MANAGED_PARTITIONS; ++i) {
		if (partitions[i].active == 0) {
			partitions[i].active = 1;
			partitions[i].lpid = 0;
			partitions[i].init_mem = init_mem;
			partitions[i].init_mem_size = init_mem_size;
			partitions[i].name = name;
			break;
		}
		if (i == MAX_MANAGED_PARTITIONS) {
			hprintf("unable to find slot to track partition\n");
			return 0;
		}
	}

	ps = &partitions[i];

	while (image_names[rc] != NULL && size == 0) {
		if (strcmp(image_names[rc], name) == 0) {
			start = (uval)image_start[rc];
			size = (uval)image_size[rc];
#ifdef DEBUG
			hprintf("%s: Found image named: %s "
				"@ 0x%lx, size 0x%lx.\n",
				__func__, name, start, size);
#endif
		}
		++rc;
	}

	if (size == 0) {
		hprintf("%s: could not find image named: %s.\n",
			__func__, name);
		return H_Parameter;
	}

#ifdef USE_LIBBZ2
	char *dcomp;
	uval dcomp_size;
	uval dcomp_buf_size;
	sval ret;
	ret = dcomp_img((char*)start, size,
			&dcomp, &dcomp_buf_size, &dcomp_size, &pa);

	if (ret >= 0) {
		hprintf("Decompressed image: %ld -> %ld\n", size, dcomp_size);
		start = (uval)dcomp;
		size = dcomp_size;
	} else {
		hprintf("Decompression failure %ld\n", ret);
	}
	/* else try to use as is, could be uncompressed */

#endif

	elftype = tryElf(&lf, start);
	if (!(elftype == KK_64 || elftype == KK_32)) {
		hputs("WARNING: Image is not elf, assuming it is a binary\n");
		lf.numberOfLoaderSegments = 1;
		lf.loaderSegmentTable[0].file_start = start;
		lf.loaderSegmentTable[0].file_size = start;
		lf.va_entry = 0;
		lf.va_tocPtr = 0;
	}


	load_image(ps, &lf, 0);

	uval id = launch_partition(ps, &lf, ofd);

#ifdef USE_LIBBZ2
	/* Free de-compression buffer, is always allocated by dcomp_img */
	free_pages(&pa, (uval)dcomp, dcomp_buf_size);
#endif

	updatePartInfo(IPC_LPAR_RUNNING, ps->lpid);


	return id;
}

uval
reload_image(const char* name, uval ofd)
{
	struct load_file_entry lf;
	uval start = 0;
	uval size = 0;
	uval rc = 0;
	int elftype;
	struct partition_status _ps;
	struct partition_status *ps=&_ps;
	int i;

	ps->lpid = H_SELF_LPID;
	ps->init_mem = 0;
	ps->init_mem_size = CHUNK_SIZE;
	ps->name = name;

	while (image_names[rc] != NULL && size == 0) {
		if (strcmp(image_names[rc], name) == 0) {
			start = (uval)image_start[rc];
			size = (uval)image_size[rc];
#ifdef DEBUG
			hprintf("%s: Found image named: %s "
				"@ 0x%lx, size 0x%lx.\n",
				__func__, name, start, size);
#endif
		}
		++rc;
	}

	if (size == 0) {
		hprintf("%s: could not find image named: %s.\n",
			__func__, name);
		return H_Parameter;
	}

#ifdef USE_LIBBZ2
	char *dcomp;
	uval dcomp_size;
	uval dcomp_buf_size;
	sval ret;
	ret = dcomp_img((char*)start, size,
			&dcomp, &dcomp_buf_size, &dcomp_size, &pa);

	if (ret >= 0) {
		hprintf("Decompressed image: %ld -> %ld\n", size, dcomp_size);
		start = (uval)dcomp;
		size = dcomp_size;
	} else {
		hprintf("Decompression failure %ld\n", ret);
	}
	/* else try to use as is, could be uncompressed */

#endif

	elftype = tryElf(&lf, start);
	if (!(elftype == KK_64 || elftype == KK_32)) {
		hputs("WARNING: Image is not elf, assuming it is a binary\n");
		lf.numberOfLoaderSegments = 1;
		lf.loaderSegmentTable[0].file_start = start;
		lf.loaderSegmentTable[0].file_size = start;
		lf.va_entry = 0;
		lf.va_tocPtr = 0;
	}

	uval load_offset = 0;
	struct loader_segment_entry *le;
	int segno = lf.numberOfLoaderSegments;
	uval begin = CHUNK_SIZE;
	uval end = 0;
	for (i = 0; i < segno; i++) {
		le = &(lf.loaderSegmentTable[i]);

		if (le->va_start < begin) {
			begin = le->va_start;
		}

		if (le->va_start + le->file_size > end) {
			end = le->va_start + le->file_size;
		}
	}

	load_offset = (uval) get_pages_aligned(&pa, end - begin, 20);
	load_offset -= begin;

	load_image(ps, &lf, load_offset);

	uval magic = lf.loaderSegmentTable[0].file_start;
	magic += 0x10;

	if (*(uval64*)magic == HYPE_PARTITION_INFO_MAGIC_NUMBER) {
		magic = *((uval*)(magic + 8));
	} else {
		magic = INVALID_LOGICAL_ADDRESS;
	}

	if (magic >= begin && magic < end) {
		load_in_lpar(ps, magic + load_offset,
			     (uval)pinfo, sizeof(pinfo));
	} else {
		hprintf("Don't know how to set up partition info "
			"not in OS image\n");
	}

	restart_partition(ps, &lf, ofd);
	assert(0, "reload_partition failed\n");

#ifdef USE_LIBBZ2
	/* Free de-compression buffer, is always allocated by dcomp_img */
	free_pages(&pa, (uval)dcomp, dcomp_buf_size);
#endif

	return H_Parameter;
}
