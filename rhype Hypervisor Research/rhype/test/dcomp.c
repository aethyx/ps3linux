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


#include <config.h>
#include <types.h>
#define BZ_NO_STDIO
#include <bzlib.h>
#include <lib.h>
#include <mmu.h>
#include <test.h>
#include <pgalloc.h>
#include <objalloc.h>

static void*
decomp_alloc(void* opaque  __attribute__((unused)), int n, int m)
{
	uval size = n * m + 8;
	void *ptr = halloc(size);
	((uval32*)ptr)[0] = 0xdeadbeef;
	((uval32*)ptr)[1] = size;

	ptr = ((uval32*)ptr) + 2;
	return ptr;
}

static void
decomp_free(void* opaque __attribute__((unused)), void *ptr)
{
	uval32 *x = (uval32*)ptr;
	x -= 2;
	assert(x[0] == 0xdeadbeef, "No deadbeef at %p\n", ptr);

	hfree(x, x[1]);

}

extern void bz_internal_error(int errcode);

void
bz_internal_error(int errcode)
{
	assert(0, "libbz2 error: %d\n", errcode);
}

extern void* malloc(uval a);
void*
malloc(uval a)
{
	assert(0, "malloc called: %lx\n", a);
	return NULL;
}

extern void free(uval a, uval b, uval c);
void
free(uval a, uval b, uval c)
{
	assert(0, "free called: %lx %lx %lx\n", a, b, c);
}

void* stderr;

extern int fprintf(void* ign, const char *fmt, ...);
int
fprintf(void* ign, const char *fmt, ...)
{
	va_list ap;
	(void)ign;
	va_start(ap, fmt);
	vhprintf(fmt, ap);
	va_end(ap);
	return 0;
}

sval
dcomp_img(char* in_start, uval in_size, char** out_start,
	  uval *mem_size, uval *data_size, struct pg_alloc *pa)
{
	bz_stream decomp;

	decomp.next_in = in_start;
	decomp.avail_in = in_size;
	decomp.total_in_lo32 = 0;
	decomp.total_in_hi32 = 0;

	// 20 MB limit
	*mem_size  = 20 * (1<<20);
	*out_start = (char*)get_pages(pa, *mem_size);

	decomp.next_out = *out_start;
	decomp.avail_out = *mem_size;
	decomp.total_out_lo32 = 0;
	decomp.total_out_hi32 = 0;


	decomp.bzalloc = decomp_alloc;
	decomp.bzfree = decomp_free;

	decomp.opaque = NULL;

	sval ret = BZ2_bzDecompressInit(&decomp, 0, 0);

	if (ret != BZ_OK) {
		hprintf("Failed to initialize decompressor: %ld\n", ret);
		return -1;
	}


	hprintf("Decompressing image.\n");
	ret = BZ2_bzDecompress(&decomp);

	if (ret != BZ_STREAM_END) {
		hprintf("Decompression failure: %ld\n", ret);
		BZ2_bzDecompressEnd(&decomp);
		ret = -1;
	}

	*data_size = decomp.total_out_lo32 |
		(((uval)decomp.total_out_hi32) << 32);
	BZ2_bzDecompressEnd(&decomp);


	//halloc_assert();

	return ret;
}

