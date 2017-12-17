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
 * Make pass-thru hcalls using /dev/hcall
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

//#define DEBUG(a...) fprintf(a)
#ifndef DEBUG
#define DEBUG(a...)
#endif

oh_hcall_args hargs;

int
main(int argc, char **argv)
{

	int x = 1;

	if (argc < 2) {
		fprintf(stderr, "No opcode specified");
		return -1;
	}

	hcall_init();

	hargs.opcode = strtoull(argv[x], NULL, 0);

	char *q = strchr(argv[x], ':');
	int num_out = OH_HCALL_NUM_ARGS;

	if (q) {
		++q;
		num_out = strtoull(q, NULL, 0);
	}
	++x;

	DEBUG(stderr, "Making hcall: 0x%llx ( ", hargs.opcode);
	while (x < argc) {
		hargs.args[x - 2] = strtoull(argv[x], NULL, 0);
		DEBUG(stderr, "%llx ", hargs.args[x - 2]);
		++x;
	}
	DEBUG(stderr, ")\n");

	int ret = hcall(&hargs);

	DEBUG(stderr, "Returned: %d %d\n", ret, errno);

	x = 0;
	printf(UVAL_CHOOSE("0x%08x", "0x%016lx") " ", hargs.retval);
	while (x < num_out) {
		printf(UVAL_CHOOSE("0x%08x", "0x%016lx") " ", hargs.args[x]);
		++x;
	}
	printf("\n");

	return ret;
}
