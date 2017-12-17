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

#include <config.h>
#include <types.h>
#include <gdbstub.h>
#include <lib.h>
#include <mmu.h>
#include <io_chan.h>
#include <test.h>

sval __gdbstub
gdb_io_write(const char *buf, uval len)
{
	return hcall_put_term_string(GDB_CHANNEL, len, buf);
}

static char gdbbuf[2 * sizeof (uval64)];
static uval start = 0;
static uval end = 0;

sval __gdbstub
gdb_io_read(char *buf, uval len)
{
	sval ret = 0;

	if (start == end) {
		start = 0;
		ret = hcall_get_term_string(GDB_CHANNEL, gdbbuf);
		if (ret < 0) {
			return ret;
		}
		end = ret;
	}

	if (start != end) {
		if (end - start <= len) {
			memcpy(buf, gdbbuf + start, end - start);
			len = end - start;
			start = end = 0;
			return len;
		} else {
			memcpy(buf, gdbbuf + start, len);
			start += len;
			return len;
		}
	}

	return ret;
}

void __gdbstub
gdb_stub_init(void)
{
}

void __gdbstub
gdb_enter_notify(uval exception_type)
{
#ifdef DEBUG
	hprintf_nlk("entering gdb: exception number 0x%lx\n", exception_type);
#else
	(void)exception_type;
#endif
}
