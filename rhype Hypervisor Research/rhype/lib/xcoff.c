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

/*
 * This is the code to parse an xcoff image.  We have this as a
 * separate file to avoid mixing the header file definitions of xcoff
 * and elf, since they define the same fields in incompatible ways.
 *
 * We are passed a pointer to an image in memory, and are to parse the
 * input file and return the locations and sizes of the text, data and
 * bss sections.
 */

#include <config.h>
#include <loadFile.h> /* I hate this file */
#include <lib.h>

struct filehdr {
        unsigned short  f_magic;
        unsigned short  f_nscns;
        sval32          f_timdat;
        sval64       f_symptr;
        unsigned short  f_opthdr;
        unsigned short  f_flags;
        sval32          f_nsyms;
};

struct aouthdr {
        short magic;
        short vstamp;
        uval32 o_debugger;
        uval64 text_start;
        uval64 data_start;
        uval64 o_toc;
        short  o_snentry;
        short  o_sntext;
        short  o_sndata;
        short  o_sntoc;
        short  o_snloader;
        short  o_snbss;
        short  o_algntext;
        short  o_algndata;
        char   o_modtype[2];
        unsigned char o_cpuflag;
        unsigned char o_cputype;
        sval32 o_resv2[1];
        sval64 tsize;
        sval64 dsize;
        sval64 bsize;
        sval64 entry;
        uval64 o_maxstack;
        uval64 o_maxdata;
        sval32 o_resv3[4];
};

struct xcoffhdr {
        struct filehdr filehdr;
        struct aouthdr aouthdr;
};

struct scnhdr {
        char  s_name[8];
        uval64 s_paddr;
        uval64 s_vaddr;
        uval64 s_size;
        sval64 s_scnptr;
        sval64 s_relptr;
        sval64 s_lnnoptr;
        uval32 s_nreloc;
        uval32 s_nlnno;
        sval32 s_flags;
	uval32 _pad;
};

/* Try to interpret the code at kernStartAddr as an XCOFF load image */
/* This is a 64-bit xcoff load image */

int
tryXcoff (struct load_file_entry *lf, uval kernStartAddr)
{
#ifdef HAS_64BIT
	const int has64 = 1;
#else
	const int has64 = 0;
#endif
	if (has64) {
		const int xcoff_magic = 0757;
		const int scnhdr_sz = sizeof (struct scnhdr);

		struct xcoffhdr *xcoffp;
		struct scnhdr *scnp;
		uval scnNum;
		uval * entryFunc;

		xcoffp = (struct xcoffhdr *) kernStartAddr;
		lf->numberOfLoaderSegments = 0;

		/* check for XCOFF magic number */
		if (xcoff_magic != xcoffp->filehdr.f_magic) {
			return(0);
		}
#ifdef DEBUG
		hprintf("Object file format is XCOFF: magic = %d\n",
			xcoffp->filehdr.f_magic);
#endif

		/* fill in the loader segment table with the 3 segments */

#define TEXT 0
#define DATA 1
#define BSS  2


		scnNum = xcoffp->aouthdr.o_sntext - 1;
		scnp = (struct scnhdr *)(((uval) &xcoffp[1]) +
					 scnNum * scnhdr_sz);
		lf->loaderSegmentTable[TEXT].va_start = scnp->s_vaddr;
		lf->loaderSegmentTable[TEXT].mem_size = scnp->s_size;
		lf->loaderSegmentTable[TEXT].file_size = scnp->s_size;
		lf->loaderSegmentTable[TEXT].file_start = 
			(uval64)(kernStartAddr + scnp->s_scnptr);
		lf->numberOfLoaderSegments += 1;

		scnNum = xcoffp->aouthdr.o_sndata - 1;
		scnp = (struct scnhdr *)(((uval) &xcoffp[1]) +
					 scnNum * scnhdr_sz);
		lf->loaderSegmentTable[DATA].va_start = scnp->s_vaddr;
		lf->loaderSegmentTable[DATA].mem_size = scnp->s_size;
		lf->loaderSegmentTable[DATA].file_size = scnp->s_size;
		lf->loaderSegmentTable[DATA].file_start =
			(uval64)(kernStartAddr + scnp->s_scnptr);
		lf->numberOfLoaderSegments += 1;

		scnNum = xcoffp->aouthdr.o_snbss - 1;
		scnp = (struct scnhdr *)(((uval) &xcoffp[1]) +
					 scnNum * scnhdr_sz);
		lf->loaderSegmentTable[BSS].va_start = scnp->s_vaddr;
		lf->loaderSegmentTable[BSS].mem_size = scnp->s_size;
		lf->loaderSegmentTable[BSS].file_size = 0;
		lf->loaderSegmentTable[BSS].file_start =
			(uval64)(kernStartAddr + scnp->s_scnptr);
		lf->numberOfLoaderSegments += 1;

		/*
		 * The entry point is specified by a function
		 * descriptor whose (data segment) virtual address is
		 * in the aux header.  The function descriptor
		 * contains: 8-byte address of first instruction
		 * 8-byte address of TOC
		 */
		entryFunc = (uval *)
			((uval)(lf->loaderSegmentTable[DATA].file_start +
				(xcoffp->aouthdr.entry -
				 lf->loaderSegmentTable[DATA].va_start)));
		lf->va_entry = entryFunc[0];
		lf->va_tocPtr = entryFunc[1];

#ifdef DEBUG
		hprintf("entry     0x%llx\n"
			"toc       0x%llx\n"
			"text\n"
			"  mem  start 0x%llx\n"
			"  mem  size  0x%llx\n"
			"  file start 0x%llx\n"
			"  file size  0x%llx\n"
			"data\n"
			"  mem  start 0x%llx\n"
			"  mem  size  0x%llx\n"
			"  file start 0x%llx\n"
			"  file size  0x%llx\n"
			"bss\n"
			"  mem  start 0x%llx\n"
			"  mem  size  0x%llx\n"
			"  file start 0x%llx\n"
			"  file size  0x%llx\n",
			lf->va_entry, lf->va_tocPtr,
			lf->loaderSegmentTable[TEXT].va_start,
			lf->loaderSegmentTable[TEXT].mem_size,
			lf->loaderSegmentTable[TEXT].file_start,
			lf->loaderSegmentTable[TEXT].file_size,
			lf->loaderSegmentTable[DATA].va_start,
			lf->loaderSegmentTable[DATA].mem_size,
			lf->loaderSegmentTable[DATA].file_start,
			lf->loaderSegmentTable[DATA].file_size,
			lf->loaderSegmentTable[BSS].va_start,
			lf->loaderSegmentTable[BSS].mem_size,
			lf->loaderSegmentTable[BSS].file_start,
			lf->loaderSegmentTable[BSS].file_size);
#endif /* DEBUG */
		return 1;
	}

	return(0);
}
