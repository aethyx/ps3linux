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
 * This is the code for supporting Zilog serial ports on Apple machines.
 * Compatible with the io_chan structure.
 *
 */

#include <config.h>
#include <lib.h>
#include <io_chan.h>
#include <io.h>

/* An incomplete set of bits defining the meaning of registers */
enum {
	CONTROL = 0,
	DATA = 0x10,
	TX_BUF_EMP = 0x4,	/* empty buffer */
	SYNC_HUNT = 0x10,	/* Sync/hunt == DSR */
	CTS = 0x20,

	ALL_SNT = 0x1,

	// Write to register 5:
	RTS = 0x2,		/* RTS */
	TX_ENABLE = 0x8,	/* tx Enable */
	SND_BRK = 0x10,		/* Send Break */
	TX_8 = 0x60,		/* 8 bits/char */
	DTR = 0x80,		/* DTR */

	// Write to register 3:
	RX_ENABLE = 0x1,
	RX_8 = 0xc0,		/* rx 8 bit/char */

	// Write to register 4:
	PAR_EVEN = 0x2,		/* parity even/odd */
	STOP_BIT1 = 0x4,	/* 1 stop bit/char */
	X16CLK = 0x40,		/* x16 clk */

	// Write to register 11:
	TX_BR_CLOCK = 0x10,	/* tx clock = br generator */
	RX_BR_CLOCK = 0x40,	/* rx clock = br generator */

	// Write to register 14:
	BR_ENABLE = 1,		/* enable br generator */

	// Read from reg 0:
	RX_CH_AV = 0x1		/* rx char Available */
};

struct zilog_chan {
	struct io_chan base;
	uval8 regs[16];
	uval comBase;
};

#define comIn(r, v) \
	(v) = io_in8((uval8*)(((uval)r)));
#define comOut(r, v) \
	io_out8((uval8*)(((uval)r)),(uval8)(v));

static void zilog_clrbit(struct io_chan *ops, uval reg, uval8 cmd);
static void zilog_setbit(struct io_chan *ops, uval reg, uval8 cmd);
static uval zilog_chkbit(struct io_chan *ops, uval reg, uval bit);

static sval zilog_write_avail(struct io_chan *ops);
static sval zilog_write(struct io_chan *ops, const char *buf, uval length);

static sval
zilog_write(struct io_chan *ops, const char *buf, uval length)
{
	struct zilog_chan *zc = (struct zilog_chan *)ops;
	uval l = 0;
#ifdef HW_FLOW_CONTROL
	while (zilog_chkbit(ops, 1, CTS) == 0) ;
#endif
	while (l < length) {
		char c = *buf++;

		while (zilog_write_avail(ops) == 0);

		comOut(zc->comBase + DATA, c);

		++l;
	}

	return length;
}

static sval
zilog_write_avail(struct io_chan *ops)
{
	if (zilog_chkbit(ops, 0, TX_BUF_EMP) && zilog_chkbit(ops, 1, ALL_SNT)) {
		return 2048;
	}
	return 0;
}

static sval
zilog_read(struct io_chan *ops, char *buf, uval length)
{
	int i = 0;
	struct zilog_chan *zc = (struct zilog_chan *)ops;

	zilog_setbit(ops, 5, DTR);
	while (length--) {
		if (zilog_chkbit(ops, 0, RX_CH_AV) == 0) {
			break;
		}

		comIn(zc->comBase + DATA, buf[i]);
		++i;
	}
	zilog_clrbit(ops, 5, DTR);
	return i;
}

static sval
zilog_read_all(struct io_chan *ops, char *buf, uval length)
{
	int i = 0;
	struct zilog_chan *zc = (struct zilog_chan *)ops;

	while (length--) {
		while (zilog_chkbit(ops, 0, RX_CH_AV) == 0) ;

		comIn(zc->comBase + DATA, buf[i]);
		++i;
	}
	return i;
}

static sval
zilog_read_avail(struct io_chan *ops)
{
	char x;
	struct zilog_chan *zc = (struct zilog_chan *)ops;

	comIn(zc->comBase + CONTROL, x);
	if (x & RX_CH_AV) {
		return 1;
	}
	return 0;
}

struct zilog_chan zilog_ic = {
	.base = {
		 .ic_write = zilog_write,
		 .ic_write_avail = zilog_write_avail,
		 .ic_read = zilog_read,
		 .ic_read_all = zilog_read_all,
		 .ic_read_avail = zilog_read_avail},
	.regs = {0,}
};

static void
zilog_setbit(struct io_chan *ops, uval reg, uval8 cmd)
{
	struct zilog_chan *zc = (struct zilog_chan *)ops;

	zc->regs[reg] |= cmd;
	comOut(CONTROL, reg);
	comOut(CONTROL, zc->regs[reg]);
}

static void
zilog_clrbit(struct io_chan *ops, uval reg, uval8 cmd)
{
	struct zilog_chan *zc = (struct zilog_chan *)ops;

	zc->regs[reg] &= ~cmd;
	comOut(CONTROL, reg);
	comOut(CONTROL, zc->regs[reg]);
}

static uval
zilog_chkbit(struct io_chan *ops, uval reg, uval bit)
{
	char c;
	struct zilog_chan *zc = (struct zilog_chan *)ops;

	if (reg != 0) {
		comOut(zc->comBase, reg);
	}
	comIn(zc->comBase + 0, c);
	return (c & bit) == bit;
}

extern void tick(uval addr, char c);

struct io_chan *
zilog_init(uval io_addr, uval32 clock, uval32 baudrate)
{
//      int wait = 0;
//      char c;
	struct zilog_chan *zc = (struct zilog_chan *)&zilog_ic;

	zc->comBase = io_addr;
	(void)clock;
	(void)baudrate;

	struct zilog_reg {
		uval8 reg;
		uval8 cmd;
	};

	(void)baudrate;
#ifndef BOOT_ENVIRONMENT_of
	// OF would have done the initialization for us already
	unsigned int j;
	struct zilog_reg init_regs[] = {
		{13, 0},	/* set baud rate divisor */
		{12, 0},
		{14, BR_ENABLE},
		{11, TX_BR_CLOCK | RX_BR_CLOCK},
		{5, TX_ENABLE | TX_8 | DTR | RTS},
		{4, X16CLK | PAR_EVEN | STOP_BIT1},
		{3, RX_ENABLE | RX_8},
		{0, 0}
	};
	// 0xc0 to register 9 --> reset
	comOut(zc->comBase + CONTROL, 9);
	comOut(zc->comBase + CONTROL, 0xC0);

	for (j = 0; init_regs[j].reg != 0; ++j) {
		zilog_ic.regs[init_regs[j].reg] = init_regs[j].cmd;
		comOut(zc->comBase + CONTROL, init_regs[j].reg);
		comOut(zc->comBase + CONTROL, init_regs[j].cmd);
	}

	/* This waits for all output to flush */
	/* OF may have output still being written out */
	while (zilog_chkbit(&zc->base, 0, TX_BUF_EMP) == 0 ||
	       zilog_chkbit(&zc->base, 1, ALL_SNT) == 0) ;
#else

#endif

	return fill_io_chan(&zilog_ic.base);

}
