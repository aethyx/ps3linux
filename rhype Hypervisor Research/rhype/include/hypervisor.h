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

#ifndef _HYPERVISOR_H
#define _HYPERVISOR_H

#include <config.h>
#include <compiler.h>

#define HYPE_LOAD_ADDRESS	0x0100000UL

/*
 * Maximum number of concurrent OSes. 
 * Current known limit is MAX_CHANNELS divided by CHANNELS_PER_OS
 * which is 23 plus change.
 */
#define MAX_OS		8

/* Per-CPU/per-OS page tables.  Linux currently uses shared page
 * table. K42 uses per-CPU/per-OS page tables. */
#undef HTAB_PER_CPU

#define HYPE_PARTITION_INFO_MAGIC_NUMBER ULL(0xBADC0FFEE0DDF00D)

#endif /* ! _HYPERVISOR_H */
