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
 * environment abstraction
 */

#ifndef _POWERPC_BOOT_H
#define _POWERPC_BOOT_H
#include <config.h>
#include <types.h>

extern void *boot_init(uval r3, uval r4, uval r5, uval r6,
		       uval r7, uval boot_msr);


struct boot_cpu_info {
	uval bci_stack;		/* must be first */
	uval bci_msr;		/* must be second */
	uval bci_ofd;
	uval bci_cpuno;
	uval bci_cpuseq;
	uval bci_spin;
};

extern uval boot_cpu_leap[];
extern void boot_smp_start(struct boot_cpu_info *ci)
	__attribute__ ((noreturn));
extern uval boot_smp_watch(struct boot_cpu_info *ci);

#include_next <boot.h>

#endif /* ! _POWERPC_BOOT_H */
