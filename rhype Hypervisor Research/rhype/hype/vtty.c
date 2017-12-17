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
#include <lib.h>
#include <hypervisor.h>
#include <hype.h>
#include <lpar.h>
#include <pmm.h>
#include <h_proto.h>
#include <vtty.h>
#include <thinwire.h>

void
vtty_init(struct os *os)
{
	int i;

	for (i = 0; i < CHANNELS_PER_OS; ++i) {
#ifdef USE_THINWIRE_IO
		uval c = i + CHANNELS_PER_OS * (1 + os->po_lpid_tag);

		os->po_chan[i] = getThinWireChannel(c);
#else
		/* munge everything to hout until we have proper vty
		 * routing */
		os->po_chan[i] = hout_get();
#endif
	}
}

sval
vtty_term_init(struct os *os, uval idx, struct io_chan *chan)
{
	assert(idx < CHANNELS_PER_OS, "channel index exceed os limit");
	assert(os->po_chan[idx] == NULL, "channel already initialized");

	os->po_chan[idx] = chan;
	return 0;
}

sval
vtty_put_term_char16(struct cpu_thread *thread, uval channel,
		     uval count, const char *data)
{
	struct os *os = thread->cpu->os;
	sval val;
	struct io_chan *c;

	assert(channel < CHANNELS_PER_OS, "channel index exceed os limit");

	c = os->po_chan[channel];
	if (c == NULL) {
#ifdef USE_THINWIRE_IO
		return H_Closed;
#else
		c = hout_get();
#endif
	}

	val = chan_write(c, data, count);

	if (val < 0) {
		return val;
	}

	assert(((uval)val) == count, "Did not write full amount");
	/*
	 * According to spec, must return H_Hardware if the physical
	 * connection to the virtual terminal is broken. But we cannot detect
	 * this, so we don't bother.
	 * Must return H_Closed if the virtual terminal is closed.
	 * Should propagate error codes from SimOSSupport to up here? - SONNY
	 */
	return H_Success;
}

sval
vtty_get_term_char16(struct cpu_thread *thread, uval channel, char *data)
{
	sval val;

#ifdef USE_THINWIRE_IO
	struct os *os = thread->cpu->os;

	assert(channel < CHANNELS_PER_OS, "channel index exceed os limit");

	if (!os->po_chan[channel]) {
		return H_Closed;
	}

	/*
	 * According to spec, must return H_Hardware if the physical
	 * connection to the virtual terminal is broken. But we cannot detect
	 * this, so we don't bother.
	 * Must return H_Closed if the virtual terminal is closed.
	 */

	val = chan_read_noblock(os->po_chan[channel], data, 16);
	if (val < 0) {
		return val;
	}
#else
	(void)channel;
	(void)data;
	val = 0;
#endif
	return_arg(thread, 1, val);

	return H_Success;
}
