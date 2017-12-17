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
 * This is the code to parse an elf file.  We have this as a separate
 * file to avoid mixing the header file definitions of xcoff and elf,
 * since they define the same fields in incompatible ways.
 *
 * We are passed a pointer to an image in memory, and are to parse the
 * input file and return the locations and sizes of the text, data and
 * bss sections.
 */

#include <config.h>
#include <loadFile.h> /* I hate this file */
#include <lib.h>

struct elf32_hdr{
	uval8  e_ident[16];
	uval16 e_type;
	uval16 e_machine;
	uval32 e_version;
	uval32 e_entry;
	uval32 e_phoff;
	uval32 e_shoff;
	uval32 e_flags;
	uval16 e_ehsize;
	uval16 e_phentsize;
	uval16 e_phnum;
	uval16 e_shentsize;
	uval16 e_shnum;
	uval16 e_shstrndx;
};

struct elf64_hdr {
	uval8  e_ident[16];
	sval16 e_type;
	uval16 e_machine;
	sval32 e_version;
	uval64 e_entry;
	uval64 e_phoff;
	uval64 e_shoff;
	sval32 e_flags;
	sval16 e_ehsize;
	sval16 e_phentsize;
	sval16 e_phnum;
	sval16 e_shentsize;
	sval16 e_shnum;
	sval16 e_shstrndx;
};
struct elf32_phdr{
	uval32 p_type;
	uval32 p_offset;
	uval32 p_vaddr;
	uval32 p_paddr;
	uval32 p_filesz;
	uval32 p_memsz;
	uval32 p_flags;
	uval32 p_align;
};

struct elf64_phdr {
	sval32 p_type;
	sval32 p_flags;
	uval64 p_offset;
	uval64 p_vaddr;
	uval64 p_paddr;
	uval64 p_filesz;
	uval64 p_memsz;
	uval64 p_align;
};

enum {
	PF_X = (1 << 0),
	PF_W = (1 << 1),
	PF_R = (1 << 2),
	PF_ALL = (PF_X | PF_W | PF_R),
	PT_LOAD = 1
};

/* Try to interpret the code at kernStartAddr as an ELF load image */
int tryElf (struct load_file_entry *lf, uval kernStartAddr)
{
	int ret = 0;
	struct elf32_hdr *e32 = (struct elf32_hdr *)kernStartAddr;

	if ((int)e32->e_ident[0] != 0x7f
	    || e32->e_ident[1] != 'E'
	    || e32->e_ident[2] != 'L'
	    || e32->e_ident[3] != 'F') {
		return(0);
	}

#ifdef DEBUG
	hputs("Object file format is ELF.\n");
#endif

	if (e32->e_machine == 20 /* EM_PPC */ ||
	    e32->e_machine == 23 /* EM_SPU */ ||
	    e32->e_machine == 3  /* EM_386 */ ) {
		struct elf32_phdr *phdr;
		struct loader_segment_entry *lst = lf->loaderSegmentTable;
		int i;
		int j;

		if (e32->e_phnum > MAX_LOADER_SEGMENTS) {
			hprintf("ERROR: to many ELF segments %d\n",
				e32->e_phnum);
			return ret;
		}

		phdr = (struct elf32_phdr *)((uval)e32 + e32->e_phoff);

		j = 0;
		for (i = 0; i < e32->e_phnum; i++) {
			if (phdr[i].p_type != PT_LOAD) {
				continue;
			}
			lst[j].file_start  = kernStartAddr +
				phdr[i].p_offset;
			lst[j].file_size = phdr[i].p_filesz;
			lst[j].va_start  = phdr[i].p_vaddr;
			lst[j].mem_size  = phdr[i].p_memsz;
			++j;
		}
		lf->va_entry = e32->e_entry;
		lf->va_tocPtr = 0;

		lf->numberOfLoaderSegments = j;
		ret = KK_32;

#ifdef HAS_64BIT
	} else if (e32->e_machine == 21 /* EM_PPC64 */) {
		/* 64 BITS */
		struct elf64_hdr *e64 = (struct elf64_hdr *)kernStartAddr;
		struct elf64_phdr *phdr;
		struct loader_segment_entry *lst = lf->loaderSegmentTable;
		int i;
		int j;

		if (e64->e_phnum > MAX_LOADER_SEGMENTS) {
			(void) hprintf("ERROR: to many ELF segments %d.\n",
				 e64->e_phnum);
			return 0;
		}

		phdr = (struct elf64_phdr *)((uval)e64 + (uval)e64->e_phoff);

		j = 0;
		for (i = 0; i < e64->e_phnum; i++) {
			if (phdr[i].p_type != PT_LOAD) {
				continue;
			}
			lst[j].file_start  = kernStartAddr +
				phdr[i].p_offset;
			lst[j].file_size = phdr[i].p_filesz;
			lst[j].va_start  = phdr[i].p_vaddr;
			lst[j].mem_size  = phdr[i].p_memsz;
			
			/* entry is a function descriptor */
			if (e64->e_entry >= phdr[i].p_vaddr &&
			    e64->e_entry <= phdr[i].p_vaddr +
			    phdr[i].p_memsz) {
				uval loc;
				uval64 *efd;

				loc = lst[j].file_start + e64->e_entry;
				loc -= phdr[i].p_vaddr;
				
				efd = (uval64 *)loc;
				
				/*
				 * This is not always correct but if
				 * this is truely a function
				 * descriptor then the 3rd double word
				 * must be 0
				 */
				if (efd[1] != 0 && efd[2] == 0) {
					lf->va_entry  = efd[0];
					lf->va_tocPtr = efd[1];
				} else {
					lf->va_entry = e64->e_entry;
					lf->va_tocPtr = 0;
				}
#if DEBUG
				hprintf("Elf64 "
					"entry PC : 0x%llx, "
					"entry TOC: 0x%llx.\n",
					lf->va_entry,
					lf->va_tocPtr);
#endif /* DEBUG */
			}
			++j;
		}

		lf->numberOfLoaderSegments = j;

		ret = KK_64;
#endif /* HAS_64BIT */
	}
	return(ret);
}
