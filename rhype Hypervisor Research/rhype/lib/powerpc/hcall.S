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
#include <config.h>
#include <asm.h>
#include <lpar.h>
#include <hypervisor.h>

#include <lib/powerpc/hcall_def.S>

/* Get the common interfaces */
#include <lib/hcall_common.S>

/* Definitions for hypervisor functions. Note that we do not use the
 * first macro arg */

HCALL(x, 1,hcall_enter, H_ENTER)
HCALL(x, 8,hcall_read, H_READ)
HCALL(x, 1,hcall_thread_control, H_THREAD_CONTROL)
HCALL(x, 0,hcall_cede, H_CEDE)

HCALL(x, 0,hcall_page_init, H_PAGE_INIT)
HCALL(x, 1,hcall_set_asr, H_SET_ASR)  /* ISTAR only. */
HCALL(x, 0,hcall_asr_on, H_ASR_ON)  /* ISTAR only. */
HCALL(x, 0,hcall_asr_off, H_ASR_OFF)  /* ISTAR only. */

HCALL(x, 8,hcall_hypervisor_data, H_HYPERVISOR_DATA)

HCALL(x, 2,hcall_get_xive, H_GET_XIVE)
HCALL(x, 0,hcall_set_xive, H_SET_XIVE)
HCALL(x, 0,hcall_htab, H_HTAB)
