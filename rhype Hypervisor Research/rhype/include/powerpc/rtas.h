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

#ifndef _RTAS_H
#define _RTAS_H

#include <types.h>

struct rtas_args_s {
	uval32 ra_token;
	uval32 ra_nargs;
	uval32 ra_nrets;
	uval32 ra_args[0];
} __attribute__ ((aligned(8)));

#define RTAS_SUCCEED		 0
#define RTAS_HW_ERROR		-1
#define RTAS_HW_BUSY		-2
#define RTAS_PARAM_ERROR	-3
#define RTAS_EXTENED_DELAY	9900	/* range is 9900-9905 */
#define RTAS_MLI		-9000	/* Multi-level isolation error */

extern uval rtas_entry __attribute__ ((weak));
extern uval rtas_instantiate(uval mem, uval msr);
extern sval32 rtas_call(struct rtas_args_s *r);

#endif /* ! _RTAS__H */
