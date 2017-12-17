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

#ifndef _POWERPC_HTAB_H
#define _POWERPC_HTAB_H

#define LOG_DEFAULT_HTAB_BYTES		20
#define DEFAULT_HTAB_BYTES		(1UL << LOG_HTAB_BYTES)

/* 256KB, from PowerPC Architecture specification */
#define HTAB_MIN_LOG_SIZE 18

#define LOG_NUM_PTES_IN_PTEG	3
#define NUM_PTES_IN_PTEG	(1 << LOG_NUM_PTES_IN_PTEG)
#define LOG_PTE_SIZE		4
#define LOG_PTEG_SIZE		(LOG_NUM_PTES_IN_PTEG + LOG_PTE_SIZE)
#define LOG_HTAB_HASH		(LOG_HTAB_SIZE - LOG_PTEG_SIZE)

/* real page number shift to create the rpn field of the pte */
#define RPN_SHIFT 		12

/* page protection bits in pp1 (name format: MSR:PR=0 | MSR:PR=1) */
#define PP_RWxx 0x0UL
#define PP_RWRW 0x2UL
#define PP_RWRx 0x4UL
#define PP_RxRx 0x6UL

#endif
