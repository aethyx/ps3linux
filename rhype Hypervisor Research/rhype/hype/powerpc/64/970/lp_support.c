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

#include <os.h>

void
restore_large_page_selection(struct cpu_thread *thread)
{
	struct os *pos = thread->cpu->os;
	union hid4 hid4;

	/* bit 61 in hid4 disables large page support in GPUL */
	/* GPUL Book IV, Table 3-8 , pg 80         */

	hid4.word = get_hid4();
	hid4.bits.lg_pg_dis = !pos->use_large_pages;
	thread->imp_regs.hid4.bits.lg_pg_dis = !pos->use_large_pages;

	/* TODO - perform required sync instructions when changing large page support */
	set_hid4(hid4.word);
}
