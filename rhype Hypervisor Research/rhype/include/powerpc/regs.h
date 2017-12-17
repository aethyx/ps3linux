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
#ifndef _POWERPC_REGS_H
#define _POWERPC_REGS_H

#define MIN_FRAME_SZ	112	/* Size of PPC64 leaf-function frame. */
#define VEC_REG_WIDTH	16

#define SPRN_VRSAVE	256
#define SPRN_DSISR	18
#define SPRN_DAR	19
#define SPRN_DEC	22
#define SPRN_SRR0	26
#define SPRN_SRR1	27
#define SPRN_SPRG0	272
#define SPRN_SPRG1	273
#define SPRN_SPRG2	274
#define SPRN_SPRG3	275

#define SPRN_SIAR	796
#define SPRN_SDAR       797

/* As defined for PU G4 */
#define SPRN_HID0      1008
#define SPRN_HID1      1009
#define SPRN_HID4      1012

#define SPRN_HID5      1014
#define SPRN_HID6      1017
#define SPRN_HID7      1018
#define SPRN_HID8      1019
#define SPRN_PIR       1023

#endif /* _POWERPC_REGS_H */
