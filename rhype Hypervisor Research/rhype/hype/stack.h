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

#ifndef _STACK_H
#define _STACK_H
#include <util.h>

#define HV_STACK_LOG_SIZE 19	/* Size of each unique HV stack 0.5M */
#define HV_STACK_SIZE	(UL(1) << HV_STACK_LOG_SIZE)
#define HV_STACK_MASK	((UL(1) << HV_STACK_LOG_SIZE) - 1)

#ifndef __ASSEMBLY__
#include <types.h>

static __inline__ uval
stack_bottom(uval sp)
{
	return ((sp & ~HV_STACK_MASK) + HV_STACK_SIZE);
}

extern uval stack_new(void);
#endif /* __ASSEMBLY__ */
#endif /* _STACK_H */
