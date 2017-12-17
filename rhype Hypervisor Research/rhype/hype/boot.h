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

#ifndef _BOOT_H
#define _BOOT_H

#include <lib.h>

struct memblock {
	uval64 addr;
	uval64 size;
};

extern struct io_chan *boot_console(void *bof);
extern uval boot_memory(void *bofp, uval img_start, uval img_end);
extern struct memblock *boot_memblock(uval i);
extern uval boot_runtime_init(void *b, uval eomem);
extern struct io_chan *boot_runtime_console(void *bofp);
extern sval boot_devtree(uval of_mem, uval sz);
extern uval boot_cpus(uval ofd);
extern sval boot_controller(uval *start, uval *end);
extern void boot_fini(void *bof);
extern void boot_shutdown(void);
extern void boot_reboot(void);

#endif /* ! _BOOT_H */
