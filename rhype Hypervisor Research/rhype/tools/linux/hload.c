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
 * Load a hunk of data into a logical addres space.
 */
#include <hype.h>
#include <hype_util.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>

static void
usage(void)
{
	printf("\n"
	       "Purpose:\n"
	       "  Logical Address Loader\n"
	       "\n"
	       "Usage: hload [OPTIONS]...\n");
	printf("   -h         --help             Print help and exit\n");
	printf("   -V         --version          Print version and exit\n");
	printf("   -fSTRING   --filename=STRING  File to load\n");
	printf("   -lLONG     --laddr=LONG       Logical address to load at\n");
	printf("   -m         --magic            "
	       "Identify magic address/pinfo (default=off)\n");
}



int
main(int argc, char **argv)
{
	char* filename = NULL;
	uval laddr = ~((uval)0);
	while (1) {
		int option_index = 0;
		char *stop_char;

		static struct option long_options[] = {
			{ "help",	0, NULL, 'h' },
			{ "filename",	1, NULL, 'f' },
			{ "laddr",	1, NULL, 'l' },
			{ "magic",	0, NULL, 'm' },
			{ NULL,	0, NULL, 0 }
		};

		stop_char = 0;
		signed char c = getopt_long (argc, argv, "hf:l:m",
					     long_options, &option_index);

		if (c == -1) break;	/* Exit from `while (1)' loop.  */

		switch (c) {
		case 'h':	/* Print help and exit.  */
			usage();
			exit(0);

		case 'f':	/* File to load.  */
			if (filename) {
				fprintf (stderr, "hload: `--filename' (`-f') "
					 "option given more than once\n");
				exit(-1);
			}
			filename = optarg;
			break;

		case 'l':	/* Logical address to load at.  */
			if (laddr != ~((uval)0)) {
				fprintf (stderr, "hload: `--laddr' (`-l') "
					 "option given more than once\n");
				exit(-1);
			}
			laddr = strtoull(optarg,NULL,0);
			break;

		default:	/* bug: option not considered.  */
			fprintf (stderr, "hload: option unknown: %c\n", c);
			usage();
			exit(-1);
		}
	}


	int fd = open("/dev/hcall", O_RDWR);


	int file = 0; /* default stdin */
	if (filename) {
		file = open(filename, O_RDONLY);
	}

	if (file < 0) {
		perror("open file:");
		return -1;
	}

	uval addr = laddr;

	uval chunk = addr & ~(CHUNK_SIZE-1);
	uval offset = addr - chunk;
	char *ptr = mmap(NULL, 64<<20, PROT_READ|PROT_WRITE, MAP_SHARED,
			 fd, chunk);

	fprintf(stderr,"Mapping: %lx -> %p %d\n", addr, ptr+offset, errno);

	if (ptr == MAP_FAILED) {
		perror("mmap");
		return -1;
	}

	int ret = 1;
	do {
		ret = read(file, ptr + offset, CHUNK_SIZE - offset);
		offset += ret;
	} while (ret > 0);


	return 0;
}
