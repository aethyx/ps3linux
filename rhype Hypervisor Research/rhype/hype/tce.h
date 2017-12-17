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

#ifndef _TCE_H
#define _TCE_H

#include <config.h>
#include <types.h>

union tce {
	uval64 tce_dword;
	/* *INDENT-OFF* */
	struct tce_bits_s {
		/* the bits here reflect the definition in Linux */
		/* the RPA considers all 52 bits to be the RPN */
		uval64 tce_cache	: 6;	
		uval64 _tce_r0		: 6; /* reserved */
		uval64 tce_rpn		:40; /* Real Page Number */

		/* The RPA considers the next 10 bits reserved */
		uval64 tce_v		: 1; /* Valid bit */
		uval64 tce_vlps		: 1; /* Valid for LPs */
		uval64 tce_lpx		: 8; /* LP index */

		/* the RPA defines the following two bits as:
		 *   00: no access
		 *   01: System Address read only
		 *   10: System Address write only
		 *   11: read/write
		 */
		uval64 tce_write	: 1;
		uval64 tce_read		: 1;
	} tce_bits;
	/* *INDENT-ON* */
};

union tce_bdesc {
	uval64 lbd_dword;
	/* *INDENT-OFF* */
	struct lbd_bits_s {
		uval64 lbd_ctrl_v	: 1;
		uval64 lbd_ctrl_vtoggle	: 1;
		uval64 _lbd_ctrl_res0	: 6;
		uval64 lbd_len		:24;
		uval64 lbd_addr 	:32;
	} lbd_bits;
	/* *INDENT-ON* */

};

struct tce_data {
	uval t_entries;
	uval t_base;
	uval t_alloc_size;
	union tce *t_tce;
};

struct os;
extern sval tce_put(struct os *os, struct tce_data *tce_data,
		    uval ioba, union tce ltce);
extern void *tce_bd_xlate(struct tce_data *tce_data, union tce_bdesc bd);

extern void tce_ia(struct tce_data *tce_data);
extern uval tce_alloc(struct tce_data *tce_data, uval base, uval size);
extern void tce_free(struct tce_data *tce_data);

#endif /* _TCE_H */
