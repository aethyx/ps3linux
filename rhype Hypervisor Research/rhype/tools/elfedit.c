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

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <types.h>

#define VERSION "0.1"

#define EI_CLASS 4
#define ELFCLASS32 1
#define ET_EXEC 2

struct elf32_hdr {
	uval8 e_ident[16];
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

struct elf32_phdr {
	uval32 p_type;
	uval32 p_offset;
	uval32 p_vaddr;
	uval32 p_paddr;
	uval32 p_filesz;
	uval32 p_memsz;
	uval32 p_flags;
	uval32 p_align;
};

static int swap;
static uval16
get16(uval16 v)
{
	if (swap) {
		uval16 val;

		val = (v & 0xff00) >> 8;
		val |= (v & 0x00ff) << 8;
		v = val;
	}
	return v;
}
static uval32
get32(uval32 v)
{
	if (swap) {
		uval32 val;

		val = (v & 0xff000000) >> 24;
		val |= (v & 0x00ff0000) >> 8;
		val |= (v & 0x0000ff00) << 8;
		val |= (v & 0x000000ff) << 24;
		v = val;
	}
	return v;
}

static int (*segfunc) (const void *, struct elf32_phdr *);
static void *segfunc_arg;

/*
 * Below are the callbacks that do the real work. Right now there's only one,
 * offset_phdr_lma.
 * To define your own:
 * - use the same prototype
 * - return MODIFIED/UNMODIFIED, and
 * - edit the getopt code in main() to select the new function
 *
 * As you can see, it will take slightly more work to write a function to edit
 * ELF *sections*, but still not very much.
 */

#define UNMODIFIED 0
#define MODIFIED   1

static int
offset_phdr_lma(const void *arg, struct elf32_phdr *phdr)
{
	signed long lma_addr = (long)arg;

	lma_addr += get32(phdr->p_paddr);

	phdr->p_paddr = get32(lma_addr);

	return MODIFIED;
}

/* end worker callbacks */

static int
edit_segments(const int phoff, const int hdrsize, const int hdrnum,
	      FILE * file)
{
	char segbuf[hdrsize];
	int i;
	int ret;
	int num_modified = 0;

	fseek(file, phoff, SEEK_SET);

	for (i = 0; i < hdrnum; i++) {
		ret = fread(segbuf, hdrsize, 1, file);
		if (ret != 1) {
			if (ret == -1) {
				perror("read segment");
			} else {
				printf("bad fread value\n");
			}
			return -1;
		}

		ret = segfunc(segfunc_arg, (struct elf32_phdr *)&segbuf);
		if (ret == UNMODIFIED) {
			/* no need to write unmodified record */
			continue;
		}

		num_modified++;

		/* seek back a record to overwrite */
		fseek(file, -hdrsize, SEEK_CUR);

		ret = fwrite(segbuf, hdrsize, 1, file);
		if (ret != 1) {
			perror("write segment");
			return -1;
		}
	}

	if (num_modified != 0) {
		printf("modified %i headers\n", num_modified);
	}

	return 0;
}

static int
read_elf(FILE * file)
{
	char elfbuf[sizeof (struct elf32_hdr)];
	struct elf32_hdr *elfhdr;
	int ret;

	ret = fread(elfbuf, sizeof (struct elf32_hdr), 1, file);
	if (ret != 1) {
		perror("fread");
		return -1;
	}

	elfhdr = (struct elf32_hdr *)&elfbuf;
	if (strncmp("\177ELF", elfhdr->e_ident, 4) != 0) {
		printf("not an ELF file\n");
		return -2;
	}

	if (elfhdr->e_ident[EI_CLASS] != ELFCLASS32) {
		printf("only support ELF32\n");
		return -2;
	}

	{
		/* so the easiest and most portable way to figure out
		 * our endian-ness is to check out the e_type (uval16)
		 * and look for ET_EXEC in the correct byte
		 * position. */
		uval8 hib = elfhdr->e_type >> 8;
		uval8 lob = elfhdr->e_type & 0xff;

		if (hib == ET_EXEC && lob != ET_EXEC) {
			/* we are not the same endian */
			swap = 1;
		} else if (hib != ET_EXEC && lob == ET_EXEC) {
			/* we are the same endian */
			swap = 0;
		} else {
			printf("only support ELF executables\n");
			return -2;
		}
	}

	if (segfunc != NULL) {
		ret = edit_segments(get32(elfhdr->e_phoff),
				    get16(elfhdr->e_phentsize),
				    get16(elfhdr->e_phnum), file);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static void
usage(const char *execname)
{
	printf("Usage: %s -s <LMA> <elf executable>\n", execname);
}

static int
get_long(const char *str, unsigned long *val)
{
	unsigned long arg;
	char *end;

	if (val == NULL)
		return -1;

	arg = strtoul(str, &end, 0);

	if (end != strchr(str, '\0') ||
	    ((arg == ULONG_MAX) && (errno == ERANGE))) {
		return -1;
	}
	*val = arg;

	return 0;
}

int
main(int argc, char *argv[])
{
	FILE *file;
	int ret;
	int ret2;

	while (-1 != (ret = getopt(argc, argv, "s:V"))) {
		switch (ret) {
		case 'V':
			printf("elfedit version %s\n", VERSION);
			return 0;
			break;
		case 's':
			ret = get_long(optarg,
				       ((unsigned long *)&segfunc_arg));
			if (ret == -1) {
				printf("bad argument '%s'\n", optarg);
				break;
			}

			segfunc = offset_phdr_lma;
			break;
		}
	}

	if ((segfunc == NULL) || (optind >= argc)) {
		usage(argv[0]);
		return -1;
	}

	file = fopen(argv[optind], "r+");
	if (file == NULL) {
		perror(argv[optind]);
		return -1;
	}

	ret = read_elf(file);

	ret2 = fclose(file);

	if (ret)
		return ret;
	else
		return ret2;
}
