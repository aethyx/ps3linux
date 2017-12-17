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
#include <debug.h>
#include <h_proto.h>
#include <htab.h>
#include <hype.h>
#include <os.h>

sval
h_htab(struct cpu_thread *thread, uval lpid, uval log_size)
{
	struct os *os;
	uval cur_htab;
	uval size = 1UL << log_size;

	DEBUG_OUT(DBG_MEMMGR, "LPAR[0x%lx] wants 0x%lx bytes htab\n", lpid, size);

	/* XXX more flexible permission checking */
	if (thread->cpu->os != ctrl_os)
		return H_Parameter;

	os = os_lookup(lpid);
	if (os == NULL)
		return H_Parameter;

	if (log_size < HTAB_MIN_LOG_SIZE) {
		hprintf("%s: LPAR[0x%lx] wanted 0x%lx bytes, but min is 0x%lx\n",
				__func__, lpid, size, 1UL << HTAB_MIN_LOG_SIZE);
		return H_Parameter;
	}

	if (log_size > HTAB_MAX_LOG_SIZE) {
		hprintf("%s: LPAR[0x%lx] wanted 0x%lx bytes, but max is 0x%lx\n",
				__func__, lpid, size, 1UL << HTAB_MAX_LOG_SIZE);
		return H_Parameter;
	}

	/* if OS already has an HTAB, don't give it another one. */
	cur_htab = GET_HTAB(os);
	if (cur_htab)
		return H_Parameter;

	htab_alloc(os, log_size);

	return H_Success;
}
