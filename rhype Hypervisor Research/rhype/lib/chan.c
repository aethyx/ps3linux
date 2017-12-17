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
/*
 * Operation vectors for I/O channels
 */
#include <config.h>
#include <lib.h>
#include <io_chan.h>
#include <lpar.h>

/* on success returns number of bytes read */
sval
chan_read(struct io_chan *chan, char* buf, uval length)
{
	sval val;
	if (!chan) return H_Hardware;
	if (!chan->ic_read) return H_Function;

	lock_acquire(&chan->lock);
	val = chan->ic_read(chan, buf, length);
	lock_release(&chan->lock);
	return val;
}

/* on success returns number of bytes read */
sval
chan_read_noblock(struct io_chan *chan, char* buf, uval length)
{
	sval val;
	if (!chan) return H_Hardware;
	if (!chan->ic_read) return H_Function;

	lock_acquire(&chan->lock);
	if (chan->ic_read_avail && !chan->ic_read_avail(chan)) {
		val = 0;
	} else {
		val = chan->ic_read(chan, buf, length);
	}
	lock_release(&chan->lock);
	return val;
}

/* on success returns number of bytes written */
sval chan_write(struct io_chan *chan, const char* buf, uval length)
{
	sval val;
	if (!chan) return H_Hardware;
	if (!chan->ic_write) return H_Function;

	lock_acquire(&chan->lock);
	val = chan->ic_write(chan, buf, length);
	lock_release(&chan->lock);
	return val;
}

/* on success returns number of bytes written */
sval chan_write_noblock(struct io_chan *chan, const char* buf, uval length)
{
	sval val;
	if (!chan) return H_Hardware;
	if (!chan->ic_write) return H_Function;

	lock_acquire(&chan->lock);
	if (chan->ic_write_avail && !chan->ic_write_avail(chan)) {
		val = 0;
	} else {
		val = chan->ic_write(chan, buf, length);
	}
	lock_release(&chan->lock);
	return val;
}

static sval
null_write(struct io_chan *ops, const char* buf, uval length)
{
	(void)ops; (void)buf;
	return length;
}

static sval
null_write_avail(struct io_chan *ops)
{
	(void)ops;
	return 1;
}

static sval
null_read(struct io_chan *ops, char* buf, uval length)
{
	(void)ops; (void)buf; (void)length;
	return 0;
}

static sval
null_read_all(struct io_chan *ops, char* buf, uval length)
{
	(void)ops; (void)buf;
	hprintf("Unimplemented read_all\n");
	breakpoint();
	return length;
}

static sval
null_read_avail(struct io_chan *ops)
{
	(void)ops;
	return 1;
}

static void
null_noread(struct io_chan *ops)
{
	(void)ops;
}

// Fills in NULL fields
struct io_chan *
fill_io_chan(struct io_chan *chan)
{
	if (!chan->ic_write) {
		chan->ic_write = null_write;
	}
	if (!chan->ic_write_avail) {
		chan->ic_write_avail = null_write_avail;
	}
	if (!chan->ic_read) {
		chan->ic_read = null_read;
	}
	if (!chan->ic_read_all) {
		chan->ic_read_all = null_read_all;
	}
	if (!chan->ic_read_avail) {
		chan->ic_read_avail = null_read_avail;
	}
	if (!chan->ic_noread) {
		chan->ic_noread = null_noread;
	}
	return chan;
}

static sval
splitter_write(struct io_chan *ops, const char *buf, uval length)
{
	sval ret;
	struct splitter_chan* sc = (struct splitter_chan*)ops;

	lock_acquire(&sc->primary->lock);
	lock_acquire(&sc->secondary->lock);
	ret = sc->primary->ic_write(sc->primary, buf, length);
	sc->secondary->ic_write(sc->secondary, buf, length);

	lock_release(&sc->secondary->lock);
	lock_release(&sc->primary->lock);
	return ret;
}

static sval
splitter_write_avail(struct io_chan *ops)
{
	struct splitter_chan* sc = (struct splitter_chan*)ops;
	sval ret;
	lock_acquire(&sc->primary->lock);
	ret = sc->primary->ic_write_avail(sc->primary) &&
		sc->secondary->ic_write_avail(sc->secondary);
	lock_release(&sc->primary->lock);
	return ret;
}

static sval
splitter_read_avail(struct io_chan *ops)
{
	struct splitter_chan* sc = (struct splitter_chan*)ops;
	sval ret;
	lock_acquire(&sc->primary->lock);
	ret = sc->primary->ic_read_avail(sc->primary);
	lock_release(&sc->primary->lock);
	return ret;
}

static sval
splitter_read(struct io_chan *ops, char *buf, uval length)
{
	struct splitter_chan* sc = (struct splitter_chan*)ops;
	sval ret;
	lock_acquire(&sc->primary->lock);
	ret = sc->primary->ic_read(sc->primary, buf, length);
	lock_release(&sc->primary->lock);
	return ret;
}

static sval
splitter_read_all(struct io_chan *ops, char *buf, uval length)
{
	struct splitter_chan* sc = (struct splitter_chan*)ops;
	sval ret;
	lock_acquire(&sc->primary->lock);
	ret = sc->primary->ic_read_all(sc->primary, buf, length);
	lock_release(&sc->primary->lock);
	return ret;
}

static void
splitter_noread(struct io_chan *ops)
{
	struct splitter_chan* sc = (struct splitter_chan*)ops;
	lock_acquire(&sc->primary->lock);
	lock_acquire(&sc->secondary->lock);
	sc->primary->ic_noread(sc->primary);
	sc->secondary->ic_noread(sc->secondary);
}

struct io_chan *
init_splitter(struct splitter_chan* sc, struct io_chan *primary,
	      struct io_chan *secondary)
{
	sc->primary = primary;
	sc->secondary = secondary;
	sc->base.ic_write = splitter_write;
	sc->base.ic_write_avail = splitter_write_avail;
	sc->base.ic_read = splitter_read;
	sc->base.ic_read_all = splitter_read_all;
	sc->base.ic_read_avail = splitter_read_avail;
	sc->base.ic_noread = splitter_noread;
	return &sc->base;
}

