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
 * Print some stuff out of partition info, after asking HV to provide
 * us with a new copy.
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
#include <string.h>
#include <sys/ioctl.h>

//#define DEBUG(a...) fprintf(a)
#ifndef DEBUG
#define DEBUG(a...)
#endif

oh_hcall_args hargs;
oh_mem_hold_args hold_args;

int
main(int argc, char **argv)
{

	int hfd = hcall_init();

	hold_args.size = 4096;

	if (argc > 1) {
		hold_args.laddr = strtoull(argv[1], NULL, 0);
	}

	if (hold_args.laddr == 0) {
		ioctl(hfd, OH_MEM_HOLD, &hold_args);
		printf("Holding " UVAL_CHOOSE("%x %x\n", "%lx %lx\n"),
		       hold_args.laddr, hold_args.size);
	}

	char *ptr = mmap(NULL, hold_args.size,
			 PROT_READ | PROT_WRITE, MAP_SHARED,
			 hfd, hold_args.laddr);

	memset(ptr, 0xff, 4096);
	hargs.opcode = H_LPAR_INFO;
	hargs.args[0] = 0x1000;
	hargs.args[1] = hold_args.laddr;

	hcall(&hargs);

	oh_partition_info_t *pinfo = (typeof(pinfo)) ptr;

	printf("pinfo.htab_size:	" UVAL_CHOOSE("%x", "%llx\n"),
	       pinfo->htab_size);
	printf("pinfo.chunk_size:	" UVAL_CHOOSE("%x", "%llx\n"),
	       pinfo->chunk_size);
	printf("pinfo.large_page_size1: " UVAL_CHOOSE("%x", "%llx\n"),
	       pinfo->large_page_size1);
	printf("pinfo.large_page_size2: " UVAL_CHOOSE("%x", "%llx\n"),
	       pinfo->large_page_size2);
	return 0;
}
