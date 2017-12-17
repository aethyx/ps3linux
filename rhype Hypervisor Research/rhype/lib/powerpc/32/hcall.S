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

#include <lib/powerpc/hcall.S>

#ifndef HAS_64BIT

/* HV is 32 bits */
HCALL(x, 0, hcall_put_term_char, H_PUT_TERM_CHAR)
HCALL(x, 5, hcall_get_term_char, H_GET_TERM_CHAR)

#else /* ! HAS_64BIT */

/* HV is 64 bits */		
HCALL(x, 0, _hcall_put_term_char, H_PUT_TERM_CHAR)
HCALL(x, _32_get_term_char, hcall_get_term_char, H_GET_TERM_CHAR)

/*
 * I know this is crazy but the 32 bit assember won't generate it for
 * us
 */
#define rldic(rA, rS, SH, ME, op) \
	.long	((30 << (31 - 5))  | \
		 (rS << (31 - 10)) | \
		 (rA << (31 - 15)) | \
		 ((SH & 0x1f) << (31 - 20)) | \
		 ((((ME & 0x1f) << 1) | (ME >> 5)) << (31 - 26)) | \
		 ( op << (31 - 29)) | \
		 ((SH >> 5) << (31 - 30)) | \
		 ( 0 << (31 - 31)))

#define rldicr(rA, rS, SH, ME) rldic(rA, rS, SH, ME, 1)
#define rldicl(rA, rS, SH, ME) rldic(rA, rS, SH, ME, 0)

C_TEXT_ENTRY(hcall_put_term_char)
	rldicr(6, 6, 32, 31)
	or	r6,r6,r7
	mr	r7, r8
	mr	r8, r9
	rldicr(7, 7, 32, 31)
	or	r7,r7,r8
	b	_hcall_put_term_char

_32_get_term_char:
ret_32_get_term_char:	/* Symbol name as mabgles by macro */
	## store the second quadword
	stw r6, 4 * REG_WIDTH(r12)
	rldicl(6, 6, 32, 32)
	stw r6, 3 * REG_WIDTH(r12)

	## store the first quadword
	stw r5, 2 * REG_WIDTH(r12)
	rldicl(5, 5, 32, 32)
	stw r5, 1 * REG_WIDTH(r12)
	STR r4, 0 * REG_WIDTH(r12)
	blr
	
#endif /* HAS_64BIT */
