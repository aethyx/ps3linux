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
#include <eic.h>
#include <lib.h>
#include <hype.h>
struct cpu_thread *
eic_exception(struct cpu_thread *thread)
{
	(void)thread;
	hprintf("No EIC defined\n");
	return NULL;
}

sval
eic_set_line(uval lpid, uval cpu, uval thr, uval hw_src, uval *isrc)
{
	(void)lpid;
	(void)cpu;
	(void)thr;
	(void)hw_src;
	(void)isrc;

	hprintf("No EIC defined\n");
	return H_UNAVAIL;
}

sval
eic_config(uval arg1, uval arg2, uval arg3, uval arg4)
{
	(void)arg1;
	(void)arg2;
	(void)arg3;
	(void)arg4;
	hprintf("No EIC defined\n");
	return H_UNAVAIL;
}
