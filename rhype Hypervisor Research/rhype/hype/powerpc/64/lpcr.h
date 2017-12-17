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
#ifndef _POWERPC_64_LPCR_H
#define _POWERPC_64_LPCR_H

#include <types.h>

union lpcr {
	struct LPCRbits {
		/* *INDENT-OFF* */
		uval64 res1:    35; /* reserved */
		uval64 rmls:     3; /* RMLS */
		uval64 res2:    14; /* reserved */
		uval64 m:        1; /* mediated external interrupt */
		uval64 tl:       1; /* SW TLB */
		uval64 res3:     6; /* reserved */
		uval64 lpes:     2; /* LPES */
		uval64 rmi:      1; /* real mode cache inhibited */
		uval64 hdice:    1; /* HDEC enable */
		/* *INDENT-ON* */
	} bits;
	uval64 word;
};

#endif /* _POWERPC_64_LPCR_H */
