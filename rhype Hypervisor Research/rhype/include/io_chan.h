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
 * Operation vectors for serial I/O, both buffer and character-at-a-time.
 */

#ifndef _IO_CHAN_H
#define _IO_CHAN_H

#include <config.h>
#include <types.h>
#include <openfirmware.h>
#include <atomic.h>

#define CONSOLE_CHANNEL	 0
#define GDB_CHANNEL	 1
#define IPCUT_CHANNEL    2
#define ZIGGYDEB_CHANNEL 3
#define IPSEL_CHANNEL    4
#define NUM_CHANNELS	 5
#define CHANNELS_PER_OS	 8


/*
 * String I/O operations.  This is for use by the thinwire
 * code and for general console-I/O usage.
 */
struct io_chan {

	/* Write the specified chars unconditionally.  May take some time. */
	sval (*ic_write) (struct io_chan * ops, const char *buf, uval length);

	/*
	 * Return the number of chars that may be written immediately.
	 * Not all devices support this notion, particularly in simulators,
	 * in which case, it is OK to lie and say you can accept some
	 * output that may in fact cause the simulator to block.
	 */
	sval (*ic_write_avail) (struct io_chan * ops);

	/* Read all of the specified chars unconditionally.
	 * May take some time.
	 *
	 */
	sval (*ic_read_all) (struct io_chan * ops, char *buf, uval length);

	/* Read the available chars, but no more than length. */
	sval (*ic_read) (struct io_chan * ops, char *buf, uval length);

	/*
	 * Return the number of chars that may be read immediately.
	 * in which case, it is OK to lie and say you can return some
	 * input that may in fact not be available yet.
	 */
	sval (*ic_read_avail) (struct io_chan * ops);

	/*
	 * Apparently, some UARTs need to be turned around from read
	 * to write and vice versa.  Very strange...  Thought that sort
	 * of thing died out in the 1970s...
	 */
	void (*ic_noread) (struct io_chan * ops);

	lock_t lock;
};

extern sval chan_read(struct io_chan *chan, char *buf, uval length);
extern sval chan_read_noblock(struct io_chan *chan, char *buf, uval length);
extern sval chan_write(struct io_chan *chan, const char *buf, uval length);
extern sval chan_write_noblock(struct io_chan *chan, const char *buf,
			       uval length);

struct io_chan *fill_io_chan(struct io_chan *chan);

/* A splitter object directs output out to "primary" and "secondary"
 * and takes all input from "primary".  Assumes that locking on the
 * splitter is sufficient, so if an io_chan is passed into a splitter
 * it had better not be used elsewhere.
 */
struct splitter_chan {
	struct io_chan base;
	struct io_chan *primary;
	struct io_chan *secondary;
};

struct io_chan *init_splitter(struct splitter_chan *sc,
			      struct io_chan *primary,
			      struct io_chan *secondary);

/* ofcon.c */
extern struct io_chan ofcon_ic;

/* uartNS16750.c */
extern struct io_chan *uartNS16750_init(uval io_addr,
					uval32 clock_freq, uval32 baudrate);
/* zilog.c */
extern struct io_chan *zilog_init(uval io_addr, uval32 clock_freq,
				  uval32 baudrate);

/* vga.c */
extern struct io_chan *vga_init(uval vbase);
extern void set_vga_base(uval base);

/* rtas */
extern struct io_chan *io_chan_rtas_init(void);

/* IOIF */
extern struct io_chan *io_chan_be_init(void) __attribute__ ((weak));

/* mambo */
extern struct io_chan *mambo_console_init(void);

extern struct io_chan *ofcon_init(void);

extern struct io_chan *mambo_thinwire_init(void);

#endif /* ! _IO_CHAN_H */
