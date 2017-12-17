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
#include <types.h>
#include <gdbstub.h>
#include <hype.h>
#include <mmu.h>
#include <io_chan.h>
#include <thinwire.h>

static struct io_chan *gdb_chan = NULL;

sval __gdbstub
gdb_io_write(const char *buf, uval len)
{
	sval ret;

	lock_acquire(&gdb_chan->lock);
	ret = gdb_chan->ic_write(gdb_chan, buf, len);
	lock_release(&gdb_chan->lock);
	return ret;
}

sval __gdbstub
gdb_io_read(char *buf, uval len)
{
	sval ret;

	lock_acquire(&gdb_chan->lock);
	ret = gdb_chan->ic_read(gdb_chan, buf, len);
	lock_release(&gdb_chan->lock);
	return ret;
}

void __gdbstub
gdb_stub_init(void)
{
#ifndef USE_THINWIRE_IO
#error "gdb stub depends on thwinire being enabled"
#endif
	gdb_chan = getThinWireChannel(GDB_CHANNEL);
}

void __gdbstub
gdb_enter_notify(uval exception_type)
{
#ifdef DEBUG
	hprintf("entering gdb: exception number 0x%lx\n", exception_type);
#else
	(void)exception_type;
#endif
}
