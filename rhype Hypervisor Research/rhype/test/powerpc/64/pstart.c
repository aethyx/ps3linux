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

#include <test.h>
#include <loadFile.h>
#include <ofd.h>
#include <lib.h>
#include <mmu.h>

int
start_partition(struct partition_status *ps, struct load_file_entry *lf,
		uval ofd)
{
	sval rc;
	uval r5;
	uval lofd = 0;

	/* give the partition an htab */

	ps->log_htab_bytes = log_htab_bytes;

	rc = hcall_htab(NULL, ps->lpid, ps->log_htab_bytes);
	assert(rc == H_Success, "hcall_htab() failed\n");

#ifdef USE_OPENFIRMWARE
	lofd = ofd_lpar_create(ps, lofd, ofd);
	assert(lofd > 0, "fail to create lpar dev devtree\n");
#else
	lofd = ofd; /* it is ignored anyway */
#endif

	r5 = prom_load(ps, lofd);

#ifdef USE_OPENFIRMWARE
	ofd_cleanup(lofd);
#endif

	rc = hcall_start(NULL, ps->lpid, lf->va_entry, lf->va_tocPtr,
			 0, 0, r5, 0, 0);
	assert(rc == H_Success, "hcall_start() failed\n");
	return 1;
}

int
restart_partition(struct partition_status* ps, struct load_file_entry* lf,
		  uval ofd)
{
	uval entry;
	uval lofd = 0;

	/* this is the size htab controller was already implicitly given */
	ps->log_htab_bytes = LOG_DEFAULT_HTAB_BYTES;

#ifdef USE_OPENFIRMWARE
	lofd = ofd_lpar_create(ps, lofd, ofd);
	assert(lofd > 0, "fail to create lpar dev devtree\n");
#else
	lofd = ofd; /* it is ignored anyway */
#endif

	entry = prom_load(ps, lofd);

	register unsigned long r3 __asm__("r3") = 0;
	register unsigned long r4 __asm__("r4") = 0;
	register unsigned long r5 __asm__("r5") = entry;
	register unsigned long r6 __asm__("r6") = 0;
	register unsigned long r7 __asm__("r7") = 0;

	asm volatile ("mtsrr0 %[pc]\n\t"
		      "mtsrr1 %[msr]\n\t"
		      "rfid\n\t"::
		      [pc] "r" (lf->va_entry),
		      [msr] "r" (MSR_ME),
		      "r" (r3),
		      "r" (r4),
		      "r" (r5),
		      "r" (r6),
		      "r" (r7));




	return 1;
}

