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

#ifndef _HYP_POWERPC_HTAB_H
#define _HYP_POWERPC_HTAB_H

#include <config.h>
#include <types.h>
#include_next <htab.h>

/* 64M: reasonable hypervisor limit? */
#define HTAB_MAX_LOG_SIZE 26

#define GET_HTAB(os) ((os)->htab.sdr1 & SDR1_HTABORG_MASK)

struct os;
extern void htab_alloc(struct os *os, uval log_size);
extern void htab_free(struct os *os);

#endif
