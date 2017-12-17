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
#include <sim_simos.h>

static sval bufSimWrite(struct io_chan *ops, const char* buf, uval len);
static sval bufSimRead(struct io_chan *ops, char* buf, uval len);

/*
 * We are on simulator.  Output is -always- available.  If output
 * space should be lacking, we simply stop the simulated CPU clock
 * until space is available, so that the code is none the wiser.  ;-)
 */
static sval
bufSimWriteAvail(struct io_chan *ops)
{
	(void) ops;
	return (1024);
}

/*
 * the bufSimRead routine *MUST* read in exactly length bytes, or
 * die trying.  The rest of the code assumes that all the
 * bytes that are requested are read in.  If you don't get
 * them all, keep trying.
 */

static sval
bufSimReadAll(struct io_chan *ops, char *buf, uval length)
{
	sval n;
	uval inlen;
	(void) ops;

	inlen = length;
	do {
		n = sim_tw_read(buf, inlen);
		assert(n > 0 && (uval)n <= inlen,
		       "recieved more chars then asked for\n");
		buf += n;
		inlen -= n;
	} while (inlen > 0);
	return (length);
}

/*
 * We are a simulator.  Input is -always- available.  If input
 * should be lacking, we simply stop the simulated CPU clock
 * until input is available, so that the code is none the wiser.  ;-)
 *
 * Note that thinwire runs a protocol that can check for pending
 * input, so, if you care, layer thinwire on top of this.
 */

static sval
bufSimReadAvail(struct io_chan *ops)
{
	(void) ops;
	return 1;
}

/*
 * nothing to do in this case to skip a read
 */

static void
bufSimNoRead(struct io_chan *ops)
{
	(void) ops;
	return;
}


static sval
bufSimRead(struct io_chan *ops, char* buf, uval len)
{
	(void) ops;
	return sim_tw_read(buf, len);
}

static sval
bufSimWrite(struct io_chan *ops, const char* buf, uval len)
{
	(void) ops;
	return sim_tw_write(buf, len);
}

static sval
conWrite(struct io_chan *ops, const char* buf, uval len)
{
	(void)ops;
	sim_write(buf, len);
	return len;
}

static struct io_chan sim_console_ic = {
	.ic_write = conWrite,
};

static struct io_chan sim_thinwire_ic ={
	.ic_write = bufSimWrite,
	.ic_write_avail = bufSimWriteAvail,
	.ic_read = bufSimRead,
	.ic_read_all = bufSimReadAll,
	.ic_read_avail = bufSimReadAvail,
	.ic_noread = bufSimNoRead
};

struct io_chan *
mambo_console_init(void)
{
	return fill_io_chan(&sim_console_ic);
}


struct io_chan *
mambo_thinwire_init(void)
{
	return fill_io_chan(&sim_thinwire_ic);
}
