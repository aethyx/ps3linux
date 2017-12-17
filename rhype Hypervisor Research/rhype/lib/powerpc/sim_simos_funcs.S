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

#include <config.h>
#include <asm.h>
#include <sim_simos.h>

C_TEXT_ENTRY(sim_memcpy)
	mr	r6, r5
	mr	r5, r4
	mr	r4, r3
	li	r3, SIM_MEMCPY
	/* need to fix return value */
	mflr	r7
	bl	C_TEXT(sim_call)
	mtlr	r7
	mr	r3, r4
	blr
C_TEXT_END(sim_memcpy)

C_TEXT_ENTRY(sim_memset)
	mr	r6, r5
	mr	r5, r4
	mr	r4, r3
	li	r3, SIM_MEMSET
	/* need to fix return value */
	mflr	r7
	bl	C_TEXT(sim_call)
	mtlr	r7
	mr	r3, r4
	blr
C_TEXT_END(sim_memset)
		
C_TEXT_ENTRY(sim_write)
	mr	r5, r4
	mr	r4, r3
	li	r3, SIM_WRITE
	li	r6, SIM_REAL_MODE
	/* need to fix return value */
	mflr	r7
	bl	C_TEXT(sim_call)
	mtlr	r7
	mr	r3, r5
C_TEXT_END(sim_write)

C_TEXT_ENTRY(sim_tw_write)
	mr	r5, r4
	mr	r4, r3
	li	r3, SIM_TW_WRITE
	b	C_TEXT(sim_call)
C_TEXT_END(sim_tw_write)

C_TEXT_ENTRY(sim_tw_read)
	mr	r5, r4
	mr	r4, r3
	li	r3, SIM_TW_READ
	b	C_TEXT(sim_call)
C_TEXT_END(sim_tw_read)
