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
 * This is the code for supporting the thinwire protocol.
 *
 * Thinwire connects to a remote thinwire server, and
 * then multiplexes multiple communications streams
 * across the one thinwire (probably a serial line).
 */

#include <config.h>
#include <lib.h>
#include <thinwire.h>		/* thinwire protocol defines */
#include <atomic.h>
#include <io_chan.h>
#include <gdbstub.h>
#include <util.h>

#include <sim.h>

/* USE_SIMPLE_WRITE == 1 --> use un-acked writes for console code */
#define USE_SIMPLE_WRITE	1
/* IMMEDIATE == 1 --> immediately activate thinwire */
#define IMMEDIATE		0


/* Magic thinwire string that enables thinwire multi-plexing protocol */
const char thinwire_magic_string[14] = "***thinwire***";

/*
 * Serial-line operations vector.
 */

static struct io_chan *thinwire_ic;
static uval thinwire_activated = 0;

/*
 * MAX_CHANNEL cannot exceed 95 as encoding adds 0x20, and the high-order
 * bit cannot be set.  Also, note that the thinwire program duplicates this
 * define.
 */
#define MAX_CHANNELS	95

static struct io_chan channels[MAX_CHANNELS];

static uval __gdbstub
getChanNum(struct io_chan *ops)
{
	uval num;

	if (ops < &channels[0] || ops >= &channels[MAX_CHANNELS])
		breakpoint();

	num = (((uval)ops) - ((uval)&channels[0])) / sizeof (struct io_chan);
	if (num >= MAX_CHANNELS) {
		breakpoint();
	}

	return num;
}

struct io_chan *
getThinWireChannel(uval num)
{
#ifdef USE_THINWIRE_IO
	assert(num < MAX_CHANNELS, "Invalid thinwire channel requested\n");
	return &channels[num];
#else
	(void)num;
	return thinwire_ic;
#endif
}


/*
 * All thinwire communications are acknowledged -- the server acks writes, and
 * we ack reads. Therefore we cannot allow interleaved reads or writes.
 */

/*
 * Parse a thinwire reply packet from an in-memory buffer.
 */

static sval __gdbstub
parseThinWireReply(uval channel, const char *buf)
{
	uval r_chan;
	sval n;

	/* check channel */
	r_chan = (buf[4] - ' ');
	if (r_chan != channel) {
		assert(0,
		       "parseThinWireReply: expected ack on channel %ld, "
		       "got %ld\n", channel, r_chan);
		return -1;
	}

	/* check length */
	n = (((buf[1] - ' ') << 12)
	     | ((buf[2] - ' ') << 6)
	     | ((buf[3] - ' ') << 0));

	return (n);
}

static sval __gdbstub thinwireWriteAvail(struct io_chan *ops);

static sval __gdbstub thinwireReadAvail(struct io_chan *ops);

static sval __gdbstub
thinwireSimpleWrite(struct io_chan *ops, const char *buf, uval length)
	__attribute__((unused));

static sval __gdbstub
thinwireRead(struct io_chan *ops, char *buf, uval max_length);

static sval __gdbstub
thinwireWrite(struct io_chan *ops, const char *buf, uval length);

static void __gdbstub
activateThinWire(void);


/* We don't setup the thinwire connection until an operation is done
 * on one of the non-console channels. We use wrapper functions so
 * that we can detect when to activate thinwire.
 * These will generate an hprintf so we can see from hype
 * output that some partition is trying to do thinwire IO.
 * The wrappers then move themselves out of the way and are
 * replaced with the regular thinwire wrappers.
 */
static const char wrap_fmt[] = "thinwire: activated channel: %ld\n";

static sval __gdbstub
wrap_thinwireRead(struct io_chan *ops, char *buf, uval max_length);

static sval __gdbstub
wrap_thinwireWrite(struct io_chan *ops, const char *buf, uval length);

static sval __gdbstub wrap_thinwireReadAvail(struct io_chan *ops);

static sval __gdbstub wrap_thinwireWriteAvail(struct io_chan *ops);

static void
tw_setops(struct io_chan *ops)
{
	if (! thinwire_activated)
		activateThinWire();

#if USE_SIMPLE_WRITE == 1
	uval i = getChanNum(ops);
	if ((i % CHANNELS_PER_OS) == CONSOLE_CHANNEL) {
		ops->ic_write = thinwireSimpleWrite;
	} else {
		ops->ic_write = thinwireWrite;
	}
#else
	ops->ic_write = thinwireWrite;
#endif

	ops->ic_read = thinwireRead;
	ops->ic_read_avail = thinwireReadAvail;
	ops->ic_write_avail = thinwireWriteAvail;
	hprintf(wrap_fmt, getChanNum(ops));
}

static sval __gdbstub
wrap_thinwireReadAvail(struct io_chan *ops)
{
	tw_setops(ops);

	return ops->ic_read_avail(ops);
}

static sval __gdbstub
wrap_thinwireWriteAvail(struct io_chan *ops)
{
	tw_setops(ops);

	return ops->ic_write_avail(ops);
}

static sval __gdbstub
wrap_thinwireWrite(struct io_chan *ops, const char *buf, uval length)
{
	tw_setops(ops);

	return ops->ic_write(ops, buf, length);
}

static sval __gdbstub
wrap_thinwireRead(struct io_chan *ops, char *buf, uval max_length)
{
	tw_setops(ops);

	return ops->ic_read(ops, buf, max_length);
}

static sval __gdbstub
thinwireSimpleWrite(struct io_chan *ops, const char *buf, uval length)
{
	uval channel = getChanNum(ops);
	char header[5];

	if (! thinwire_activated) {
		thinwire_ic->ic_write(thinwire_ic, buf, length);
		return length;
	}

	/*
	 * to write on thinwire, we output
	 * 0  -- write command
	 * nnn - base-64 length of buffer (000 to 63.63.63)
	 * x   - channel number  (ASCII, 0 to 95)
	 *
	 * then nnn bytes of data
	 */

	header[0] = '#';     /*'#'indicates "write" with no ACK */
	header[1] = ' ' + ((length >> 12) & 0x3f);
	header[2] = ' ' + ((length >>  6) & 0x3f);
	header[3] = ' ' + ((length >>  0) & 0x3f);
	header[4] = ' ' + channel;

	lock_acquire(&thinwire_ic->lock);

	/* write the header */
	thinwire_ic->ic_write(thinwire_ic, header, sizeof(header));

	/* write the buffer */
	thinwire_ic->ic_write(thinwire_ic, buf, length);

	lock_release(&thinwire_ic->lock);
	return length;
}

static sval __gdbstub
thinwireWrite(struct io_chan *ops, const char *buf, uval length)
{
	uval channel = getChanNum(ops);
	sval inlen;
	char header[5];

	if (! thinwire_activated) {
		thinwire_ic->ic_write(thinwire_ic, buf, length);
		return length;
	}

	/*
	 * to write on thinwire, we output
	 * 0  -- write command
	 * nnn - base-64 length of buffer (000 to 63.63.63)
	 * x   - channel number  (ASCII, 0 to 95)
	 *
	 * then nnn bytes of data
	 */

	header[0] = '0';	/*'0'indicates "write" */
	header[1] = ' ' + ((length >> 12) & 0x3f);
	header[2] = ' ' + ((length >> 6) & 0x3f);
	header[3] = ' ' + ((length >> 0) & 0x3f);
	header[4] = ' ' + channel;

	/* take a lock to guarantee that the write/read is not interrupted */
	if (!lock_held(&ops->lock)) {
		/* thinwire won't work interleaved, so attempts to
		 * bypass locking will probably end in
		 * deadlock. Unfortunately we can't panic() because
		 * that will use thinwire and deadlock too. The best
		 * we can do is write directly to an assumed UART and
		 * hope someone can read it.
		 */
		static const char panicstr[] =
			"TWPANIC!TWPANIC!TWPANIC no lock!\r\n";
		thinwire_ic->ic_write(thinwire_ic,
				      panicstr, sizeof (panicstr) - 1);
		breakpoint();
	}
	lock_acquire(&thinwire_ic->lock);

	/* write the header */
	thinwire_ic->ic_write(thinwire_ic, header, sizeof (header));

	/* write the buffer */
	thinwire_ic->ic_write(thinwire_ic, buf, length);

	/*
	 * read an acknowledgement back
	 *
	 * the acknowledgement is 'A' (1 byte), the number of bytes
	 * received (nnn - 3 bytes), and the channel (1 byte)
	 */

	thinwire_ic->ic_read_all(thinwire_ic, header, sizeof(header));

	/* check that the reply matches the message */

	inlen = parseThinWireReply(channel, header);
	assert(inlen != -1 && (uval)inlen == length,
	       "thinwireWrite: expected ack length %ld got %ld\n",
	       length, inlen);

	lock_release(&thinwire_ic->lock);
	return length;
}

static sval __gdbstub
thinwireWriteAvail(struct io_chan *ops)
{
	(void)ops;
	return (thinwire_ic->ic_write_avail(thinwire_ic));
}

static
uval32 __gdbstub
locked_thinwireSelect(uval base)
{
	uval length = 0;
	uval32 avail;

	char buffer[13];

	/* '?' indicates "select" */
	buffer[0] = '?';
	buffer[1] = ' ' + ((length >> 12) & 0x3f);
	buffer[2] = ' ' + ((length >> 6) & 0x3f);
	buffer[3] = ' ' + ((length >> 0) & 0x3f);
	buffer[4] = ' ' + base;

	/* write the header asking for select information */
	thinwire_ic->ic_write(thinwire_ic, buffer, 5);

	/* read the select information */
	thinwire_ic->ic_read_all(thinwire_ic, buffer, sizeof (buffer));

	avail = 0;
	/* select info is encoded big-endian, in buffer bytes 5 - 12 */
	avail |= (buffer[5] - ' ') << 28;
	avail |= (buffer[6] - ' ') << 24;
	avail |= (buffer[7] - ' ') << 20;
	avail |= (buffer[8] - ' ') << 16;
	avail |= (buffer[9] - ' ') << 12;
	avail |= (buffer[10] - ' ') << 8;
	avail |= (buffer[11] - ' ') << 4;
	avail |= (buffer[12] - ' ');

	thinwire_ic->ic_noread(thinwire_ic);

	return (avail);
}

static sval __gdbstub
thinwireRead(struct io_chan *ops, char *buf, uval max_length)
{
	uval channel = getChanNum(ops);
	char header[5];
	sval inlen;
	sval r_length = 0;

	if (! thinwire_activated) {
		return thinwire_ic->ic_read(thinwire_ic, buf, max_length);
	}

	/* take a lock to guarantee that the write/read is not interrupted */
	if (!lock_held(&ops->lock)) {
		/* see comment in Write() */
		static const char panicstr[] =
			"TRPANIC!TRPANIC!TRPANIC no lock!\r\n";
		thinwire_ic->ic_write(thinwire_ic,
				      panicstr, sizeof (panicstr) - 1);
		breakpoint();
	}
	lock_acquire(&thinwire_ic->lock);

	if (!locked_thinwireSelect(channel))
		goto abort;

	header[0] = 'A';	/* 'A' indicates "read" */
	header[1] = ' ' + ((max_length >> 12) & 0x3f);
	header[2] = ' ' + ((max_length >> 6) & 0x3f);
	header[3] = ' ' + ((max_length >> 0) & 0x3f);
	header[4] = ' ' + channel;

	/* write the header */
	thinwire_ic->ic_write(thinwire_ic, header, sizeof (header));

	/* read the reply header */
	inlen = 0;
	thinwire_ic->ic_read_all(thinwire_ic, header, sizeof (header));

	/* check that it is our reply */
	r_length = parseThinWireReply(channel, header);
	assert((uval)r_length <= max_length,
	       "expected at most %ld, got %ld\n", max_length, r_length);

	if (r_length > 0)
		thinwire_ic->ic_read_all(thinwire_ic, buf, (uval)r_length);

	thinwire_ic->ic_noread(thinwire_ic);
/* *INDENT-OFF* */
abort:
/* *INDENT-ON* */

	lock_release(&thinwire_ic->lock);

	return (r_length);
}

static sval __gdbstub
thinwireReadAvail(struct io_chan *ops)
{
	uval channel = getChanNum(ops);
	uval ret;

	if (! thinwire_activated) {
		sval d = thinwire_ic->ic_read_avail(thinwire_ic);
		return d;
	}

	lock_acquire(&thinwire_ic->lock);
	ret = ((locked_thinwireSelect(channel) & 0x1) != 0);
	lock_release(&thinwire_ic->lock);

	return ret;
}


void
resetThinwire()
{
#ifdef USE_THINWIRE_IO
	lock_acquire(&thinwire_ic->lock);

	if (!onsim()) {
		/* Purge all garbage on the line */
		char garbage[32];

		while (thinwire_ic->ic_read(thinwire_ic, garbage, 32) == 32) ;
	}

	thinwire_ic->ic_write(thinwire_ic, thinwire_magic_string,
			      sizeof (thinwire_magic_string));

	lock_release(&thinwire_ic->lock);
#endif /* USE_THINWIRE_IO */
}


void
configThinWire(struct io_chan *ic)
{

#ifndef USE_THINWIRE_IO
	thinwire_ic = ic;
	return;
#endif

	int immediate = IMMEDIATE;
	int i = 0;
	struct io_chan chan;

	thinwire_ic = ic;
	memset(&chan, 0, sizeof (chan));

	lock_init(&chan.lock);

	if (immediate) {
		activateThinWire();
		for (i = 0; i < MAX_CHANNELS; i++) {
			ic = &channels[i];

#if USE_SIMPLE_WRITE == 1
			if ((i % CHANNELS_PER_OS) == CONSOLE_CHANNEL) {
				ic->ic_write = thinwireSimpleWrite;
			} else {
				ic->ic_write = thinwireWrite;
			}
#else
			ic->ic_write = thinwireWrite;
#endif
			ic->ic_write_avail = thinwireWriteAvail;
			ic->ic_read = thinwireRead;
			ic->ic_read_avail = thinwireReadAvail;

			fill_io_chan(ic);
		}
		return;
	}



	/* For all non-console channels, use wrapper functions for now.
	 * These will activate thinwire and generate an hprintf so we
	 * can see from hype output that some partition is trying to
	 * do thinwire IO. The wrappers then move themselves out of
	 * the way and are replaced with the regular thinwire wrappers.
	 */
	for (i = 0; i < MAX_CHANNELS; i++) {
		ic = &channels[i];

#if USE_SIMPLE_WRITE == 1
		if ((i % CHANNELS_PER_OS) == CONSOLE_CHANNEL) {
			ic->ic_write = thinwireSimpleWrite;
		} else {
			ic->ic_write = wrap_thinwireWrite;
		}
#else
		ic->ic_write = wrap_thinwireWrite;
#endif
		ic->ic_write_avail = wrap_thinwireWriteAvail;
		ic->ic_read = wrap_thinwireRead;
		ic->ic_read_avail = wrap_thinwireReadAvail;

		fill_io_chan(ic);
	}


	/* until thinwire_activated, pass-through reads to first partition */
	ic = &channels[CHANNELS_PER_OS + CONSOLE_CHANNEL];
	ic->ic_read = thinwireRead;
	ic->ic_read_avail = thinwireReadAvail;

}

struct io_chan *(*serial_init_fn) (uval io_addr, uval32 clock,
				   uval32 baudrate);

#define __STR(x) #x
#define STRINGIFY(x) __STR(x)
static void __gdbstub
activateThinWire(void)
{
	thinwire_activated = 1;
	resetThinwire();

#ifdef THINWIRE_BAUDRATE
	char buf[32];
	int j = 0;
	int i = 0;
	const char *speed = STRINGIFY(THINWIRE_BAUDRATE);

	if (speed == NULL || onsim()) {
		return;
	}

	buf[i++] = 'S';
	buf[i++] = ' ';
	buf[i++] = ' ';
	buf[i++] = ' ' + strlen(speed);
	buf[i++] = ' ';

	while (speed[j]) {
		buf[i++] = speed[j++];
	}

	thinwire_ic->ic_write(thinwire_ic, buf, i);

	thinwire_ic->ic_read_all(thinwire_ic, buf, 5);

	int len = parseThinWireReply(0, buf);

	if (len < 0) {
		/* We were rejected */
		return;
	}

	thinwire_ic->ic_read_all(thinwire_ic, buf+5, len);
	len += 5;

	if (memcmp(speed, buf + 5, strlen(speed)) != 0) {
		/* We were rejected */
		return;
	}

	/* Change speed */
	thinwire_ic = (*serial_init_fn)(0, 0, THINWIRE_BAUDRATE);

	uval x = 1<<20;
	while (x--);

	/* Wait for last message to be replayed at new speed */
	thinwire_ic->ic_read_all(thinwire_ic, buf, i);


#endif

}
