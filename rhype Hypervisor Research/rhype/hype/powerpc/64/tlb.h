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

#ifndef _POWERPC_64_TLB_H
#define _POWERPC_64_TLB_H

#include <config.h>
#include <types.h>

struct tlb_vpn {
	uval64 word;
	struct vpn_bits {
		/* *INDENT-OFF* */

		uval64 avpn:	57;	/* abreviated VPN */
		uval64 sw:       4;     /* reserved for SW */
		uval64 l:        1;     /* large page */
		uval64 h:        1;     /* hash function ID */
		uval64 v:        1;     /* valid */
		/* *INDENT-ON* */
	} bits;
};

union tlb_index {
	uval64 word;
	struct index_bits {
		/* *INDENT-OFF* */

		uval64 w:        1;     /* HW managed TLB */
		uval64 res:      15;    /* reserved*/
		uval64 lvpn:     11;    /* low-order VPN bits */
		uval64 index:    37;    /* index */
		/* *INDENT-ON* */
	} bits;
};

#endif
