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

#ifndef _OFD_H
#define _OFD_H

#include <types.h>
#include <of_devtree.h>
#include <test.h>

struct range {
	uval64 base;
	uval64 size;
	uval64 end;
};
extern uval ofd_pci_range_register(struct of_pci_range64_s *r64,
				   uval num, uval cells, uval lpid, uval buid,
				   struct range *r, uval mmax, uval mmin);
extern uval ofd_devtree_init(uval mem, uval *space);
extern uval ofd_lpar_create(struct partition_status *ps, uval new, uval mem);
extern void ofd_proc_props(void *m, uval32 lpid);
extern ofdn_t ofd_per_proc_props(void *m, ofdn_t n, uval32 lpid);
extern void ofd_proc_dev_probe(void *m);
extern void ofd_bootargs(uval ofd, char *buf, uval sz);
extern uval ofd_cleanup(uval ofd); /* clean up after ofd_lpar_create */

/* HW specific configuration */
extern void ofd_platform_probe(void* ofd) __attribute__((weak));
extern void ofd_platform_config(struct partition_status* ps, void* ofd)
	__attribute__((weak));

extern void ofd_openpic_probe(void *m, uval little_endian);

extern uval64 ofd_pci_addr(uval mem);
extern uval config_hba(void *m, uval lpid);

#endif /* _OFD_H */
