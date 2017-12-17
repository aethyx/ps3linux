/*
 * Copyright (C) 2005 Michal Ostrowski <mostrows@watson.ibm.com>, IBM Corp.
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
 * Read a hunk of data from a logical addres space, dump to stdout.
 *
 * $Id$
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#include <hype.h>
#include <hype_util.h>

static void
usage()
{
	fprintf(stderr,
		"Usage:\n"
		"	hread <logical_addr>:<size>\n"
		" Reads logical memory, writes contents to stdout\n"
		" Valid arguments include:\n"
		"      	0x8000000:0x4000000\n"
		"	0x80MB:64MB\n"
		"	2GB:1GB\n");
}


int
main(int argc, char **argv)
{


	int fd = open("/dev/hcall", O_RDWR);


	if (argc < 1) {
		usage();
		return -1;
	}

	uval addr;
	uval len;

	if (parse_laddr(argv[1], &addr, &len) != 0 ) {
		fprintf(stderr, "Bad argument format\n");
		usage();
		exit(-1);
	}

	uval64 chunk = addr & ~(CHUNK_SIZE-1);
	uval64 offset = addr - chunk;
	char *ptr = mmap(NULL, CHUNK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED,
			 fd, chunk);

	if (ptr == MAP_FAILED) {
		perror("mmap");
		return -1;
	}

	write(1, ptr + offset, len);

	return 0;
}
