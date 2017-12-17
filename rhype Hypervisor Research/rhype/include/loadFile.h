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

#ifndef __LOADFILE_H_
#define __LOADFILE_H_

/* The header file with the prototypes for hype_boot */

/* get basic types */
#include <config.h>
#include <types.h>

/*
  The result of loading is a list of loader memory segments.
  Each loader memory segment has 
  
  (a) a start and size in the "file"
  (b) a start and size in memory.
  
  This represents a segment which is currently at the "file"
  location, but should be moved to the (virtual) address
  given by the memory address.  If the "file" size is less
  than the memory size, then the extra should be zeroed.
*/

#define MAX_SEGMENT_NAME_LENGTH 16
struct loader_segment_entry {
	uval64 file_start;
	uval64 file_size;
	uval64 va_start;
	uval64 mem_size;
};

/* we allow up to 16 of these memory segements */
#define MAX_LOADER_SEGMENTS 16

struct load_file_entry {
	struct loader_segment_entry loaderSegmentTable[MAX_LOADER_SEGMENTS];
	int numberOfLoaderSegments;

	int kernelKind;
	uval64 pa_memSize;	/* total amount of memory in system */
	uval64 pa_minText;	/* minimum text/data/bss address */
	uval64 pa_minMemory;	/* minimum physical address of program */
	uval64 pa_maxMemory;	/* maximum physical address */
	uval64 vMapsRDelta;	/* delta between virtual and physical */
	uval64 va_minMemory;	/* minimum virtual address */
	uval64 va_maxMemory;	/* maximum virtual address */

#define PHYSICAL_ADDR(lf,v) ((v) - lf->vMapsRDelta)
#define VIRTUAL_ADDR(lf,p)  ((p) + lf->vMapsRDelta)

	uval64 pa_stack;	/* physical address of the stack */
	uval64 pa_rtas;
	uval64 va_entry;	/* virtual address of entry point */
	uval64 va_tocPtr;	/* virtual address of TOC (table of contents) */
};

/* routines to parse object file formats */
extern int tryXcoff(struct load_file_entry *lf, uval saddr);
extern int tryElf(struct load_file_entry *lf, uval saddr);

#define KK_64			0x00000001
#define KK_BOOT_UNMAPPED	0x00000002
#define KK_EXCEPTION_UNMAPPED	0x00000004
#define KK_HTAB_PER_CPU		0x00000008
#define KK_LINUX_BOLTED		0x00000010
#define KK_MAP_KERNEL_LOW	0x00000020
#define KK_32			0x00000040
#define KK_K42			0x00000080

#endif /* __LOADFILE_H_ */
