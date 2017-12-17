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
 * Probe the partition's logical address space.
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

//#define DEBUG(a...) fprintf(a)
#ifndef DEBUG
#define DEBUG(a...)
#endif

oh_hcall_args hargs;

int
main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	uval start = 0;

	hcall_init();

	hargs.opcode = H_LPAR_INFO;
	do {
		hargs.args[0] = 0x2000;
		hargs.args[1] = start;

		hcall(&hargs);

		if (0 > (sval)hargs.retval) {
			printf("Making hcall: " UVAL_CHOOSE("%x", "%lx") "\n",
			       hargs.retval);
			exit(1);
		}

		if (hargs.args[0] != ~((uval)0)) {

			printf(UVAL_CHOOSE("0x%08x[0x%08x]\n",
					   "0x%08lx[0x%08lx]\n"),
			       hargs.args[0], hargs.args[1]);
		}

		start = hargs.args[0] + hargs.args[1] + 1;
	} while (hargs.args[0] != ~((uval)0));

	return 0;
}
