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
#ifndef _POWERPC_64_GPUL_HV_REGS_H
#define _POWERPC_64_GPUL_HV_REGS_H

#include <powerpc/64/hv_regs.h>

#ifndef __ASSEMBLY__
#include <config.h>
#include <types.h>
#include <hid.h>

static inline void
mtlpidr(uval val)
{
	union hid4 hid4 = {.word = get_hid4() };
	hid4.bits.lpid01 = val & 3;
	hid4.bits.lpid25 = (val >> 2) & 0xf;
	set_hid4(hid4.word);
}

static inline void
set_hrmor(uval val)
{
	union hid5 hid5 = {.word = get_hid5() };
	hid5.bits.hrmor = val;
	set_hid5(hid5.word);
}

static inline uval
get_hrmor(void)
{
	union hid5 hid5 = {.word = get_hid5() };
	return hid5.bits.hrmor;
}

#endif /* ! __ASSEMBLY__ */

#endif /* ! _POWERPC_64_GPUL_HV_REGS_H */
