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
#include <h_proto.h>
#include <hype.h>
#include <hcall.h>
#include <debug.h>
#include <thinwire.h>
#include <os.h>

#ifdef DEBUG

uval
probe_reg(struct cpu_thread *thread, uval idx)
{
#define MAP_REG(idx, name) case idx: return thread->reg_##name;
	switch (idx) {
	case 0 ... 31:
		return thread->reg_gprs[idx];
#ifdef HAS_HDECR
	MAP_REG(32, hsrr0);
	MAP_REG(33, hsrr1);
#endif
#ifdef HAS_ASR
	MAP_REG(34, asr);
#endif
#ifdef HAS_FP
	MAP_REG(35, fpscr);
#endif
	MAP_REG(36, sprg[0]);
	MAP_REG(37, sprg[1]);
	MAP_REG(38, sprg[2]);
	MAP_REG(39, sprg[3]);

	MAP_REG(40, lr);
	MAP_REG(41, ctr);
	MAP_REG(42, srr0);
	MAP_REG(43, srr1);
	MAP_REG(44, dar);
	default:
		return 0;
	}
	return 0;
}


#endif /* DEBUG */
