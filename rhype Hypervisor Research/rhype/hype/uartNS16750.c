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
 * This is the code for supporting IBM variant of the NS16750 UART.
 * Compatible with the io_chan structure.
 *
 */

#include <config.h>
#include <lib.h>
#include <io.h>
#include <util.h>
#include <thinwire.h>
#include <asm.h>
#include <mmu.h>

#define RBR 0			/* Receiver Buffer Register */
#define THR 0			/* Transmitter Holding Register */
#define IER 1			/* Interrupt Enable Register */
#define IIR 2			/* Interrupt Identification Register */
#define FCR 2			/* FIFO Control Register */
#define LCR 3			/* Line Control Register */
#define MCR 4			/* Modem Control Register */
#define BD_UB 1			/* Baudrate Divisor - Upper Byte */
#define BD_LB 0			/* Baudrate Divisor - Lower Byte */
#define LSR 5			/* Line Status Register */
#define MSR 6			/* Modem Status Register */

#define LCR_BD 0x80		/* LCR value to enable baudrate change */
#define LCR_8N1 0x03		/* LCR value for 8-bit word, no parity, 1 stop bit */
#define MCR_DTR 0x01		/* MCR value for Data-Terminal-Ready */
#define MCR_RTS 0x02		/* MCR value for Request-To-Send */
#define MCR_OUT1 0x4
#define MCR_OUT2 0x8
#define LSR_THRE 0x20		/* LSR value for Transmitter-Holding-Register-Empty */
#define LSR_DR 0x01		/* LSR value for Data-Ready */
#define MSR_DSR 0x20		/* MSR value for Data-Set-Ready */
#define MSR_CTS 0x10		/* MSR value for Clear-To-Send */
#define FCR_FE  0x01		/* FIFO Enable */

#define comIn(r, v) \
	(v) = io_in8((uval8*)(uart->comBase+(r)));
#define comOut(r, v) \
	io_out8((uval8*)(uart->comBase+(r)),(uval8)(v));

/* Base address for UART. */

/* assumes a 1.8432 MHz clock */
unsigned std_divisors[][2] = {
	{50, 2304},
	{75, 1536},
	{110, 1047},
#ifdef FRACTIONAL_BAUD
	{134.5, 857},		/* sorry, cannot express fractions */
#endif
	{150, 768},
	{300, 384},
	{600, 192},
	{1200, 96},
	{2400, 48},
	{4800, 24},
	{9600, 12},
	{19200, 6},
	{38400, 3},
	{57600, 2},
	{115200, 1},
	{0, 0}			/* end marker */
};

static sval uartNS16750_write_avail(struct io_chan *ops);
static sval uartNS16750_write(struct io_chan *ops, const char *buf, uval len);
static sval uartNS16750_read_avail(struct io_chan *ops);
static sval uartNS16750_read(struct io_chan *ops, char *buf, uval len);
static sval uartNS16750_read_all(struct io_chan *ops, char *buf, uval len);
static void uartNS16750_noread(struct io_chan *ops);

struct uart {
	struct io_chan ops;
	uval comBase;
};

struct uart uartNS16750 = {
	.ops = {
		.ic_write = uartNS16750_write,
		.ic_write_avail = uartNS16750_write_avail,
		.ic_read = uartNS16750_read,
		.ic_read_all = uartNS16750_read_all,
		.ic_read_avail = uartNS16750_read_avail,
		.ic_noread = uartNS16750_noread}
};

#ifdef DEBUG
static void
uartNS16750_debug_status(struct io_chan *ops)
{
	struct uart *uart = (struct uart *)ops;
	char settings[7];

	comIn(IIR, settings[0]);
	comIn(LCR, settings[1]);
	comIn(MCR, settings[2]);
	comIn(LSR, settings[3]);
	comIn(MSR, settings[4]);

	comOut(LCR, settings[1] + LCR_BD);

	comIn(BD_LB, settings[5]);
	comIn(BD_UB, settings[6]);

	comOut(LCR, settings[1]);

	hprintf("settings for %3lx:IIR %02x LCR %02x MCR %02x LSR %02x \n"
		"                 MSR %02x BD_LB %02x BD_UB%02x\n",
		uart->comBase & 0xfffLU,
		settings[0],
		settings[1],
		settings[2],
		settings[3], settings[4], settings[5], settings[6]);

}
#endif /* DEBUG */


struct io_chan *
uartNS16750_init(uval io_addr, uval32 waitDSR, uval32 baudrate)
{
	struct uart *uart = &uartNS16750;


	if (io_addr == 0 && waitDSR == 0) {
		baudrate = 115200 / baudrate;
		comOut(LCR, 0);
		comOut(IER, 0xff);
		comOut(IER, 0x0);
		comOut(LCR, LCR_BD);
		comOut(BD_LB, baudrate & 0xff);
		comOut(BD_UB, baudrate >> 8);
		comOut(LCR, LCR_8N1);
		comOut(MCR, MCR_RTS|MCR_DTR);
		comOut(FCR, FCR_FE);
		return fill_io_chan(&uartNS16750.ops);
	}

	/* Serial port address is relative to comBase */
	if (io_addr) {
		uart->comBase = io_addr;
	}

	/*
	 * Wait for Data-Set-Ready on COM1.
	 */

	if (waitDSR) {
		while (1) {
			uval32 msr;

			comIn(MSR, msr);
			if ((msr & MSR_DSR) == MSR_DSR) {
				break;
			}
		}
	}

	/*
	 * Set baudrate and other parameters, and raise Data-Terminal-Ready.
	 */
	if (baudrate != 0) {
		baudrate = 115200 / baudrate;
		comOut(LCR, 0);
		comOut(IER, 0xff);
		comOut(IER, 0x0);
		comOut(LCR, LCR_BD);
		comOut(BD_LB, baudrate & 0xff);
		comOut(BD_UB, baudrate >> 8);
		comOut(LCR, LCR_8N1);
		comOut(MCR, MCR_RTS|MCR_DTR);
		comOut(FCR, FCR_FE);
	}
	comOut(MCR, MCR_RTS|MCR_DTR);

//#define TEST_SERIAL_PORT
#ifdef TEST_SERIAL_PORT
	int count = 0;
	while (1) {
		char c;

		int x = 1<<24;
		uval32 lsr;
		comOut(THR,'a');
		while (x--) eieio();
		comIn(LSR, lsr);
		if ((count % 4) == 0) {
			comOut(MCR, 0);
		}
		if ((count % 4) == 1) {
			comOut(MCR, MCR_DTR);
		}
		if ((count % 4) == 2) {
			comOut(MCR, MCR_RTS);
		}
		if ((count % 4) == 3) {
			comOut(MCR, MCR_RTS|MCR_DTR);
		}
		++count;
		if ((lsr & LSR_DR) == LSR_DR) {
			comIn(RBR, c);
			comOut(THR,c);
			comOut(THR,'a');
		} else {
			comOut(THR,'b');
		}
	}
#endif

#ifdef DEBUG
	if (io_addr) {
		uartNS16750_debug_status(&uart->ops);
	}
#endif

	return fill_io_chan(&uartNS16750.ops);
}

static sval
uartNS16750_write(struct io_chan *ops, const char *buf, uval len)
{
	struct uart *uart = (struct uart *)ops;
	uval32 lsr;
	uval l = 0;

	while (l < len) {
		char c = *buf++;

		do {
			comIn(LSR, lsr);
		} while ((lsr & LSR_THRE) != LSR_THRE);
		comOut(THR, c);

		++l;

	}
	do {
		comIn(LSR, lsr);
	} while ((lsr & LSR_THRE) != LSR_THRE);
	return l;
}

static sval
uartNS16750_write_avail(struct io_chan *ops)
{
	struct uart *uart = (struct uart *)ops;
	uval32 lsr;

	comIn(LSR, lsr);
	if (((lsr & LSR_THRE) == LSR_THRE)) {
		return 2048;
	}
	return 0;
}

static sval
uartNS16750_read_all(struct io_chan *ops, char *buf, uval len)
{
	struct uart *uart = (struct uart *)ops;
	uval32 lsr;
	char c;
	uval l = 0;
	while (l < len) {
		while (1) {
			comIn(LSR, lsr);
			if (((lsr & LSR_DR) == LSR_DR)) break;
		}
		comIn(RBR, c);
		*buf++ = c;
		++l;
	}
	return l;
}

static sval
uartNS16750_read(struct io_chan *ops, char *buf, uval len)
{
	struct uart *uart = (struct uart *)ops;
	uval32 lsr;
	char c;
	uval l = 0;
	uval x;

	while (l < len) {
		x = 1<<16;
		while (x--) {
			comIn(LSR, lsr);
			if ((lsr & LSR_DR) == LSR_DR) break;
		}

		if (((lsr & LSR_DR) == 0)) break;
		comIn(RBR, c);

		*buf++ = c;
		++l;
	}

	return l;
}

static sval
uartNS16750_read_avail(struct io_chan *ops)
{
	struct uart *uart = (struct uart *)ops;
	uval32 lsr;

	comIn(LSR, lsr);
	return ((lsr & LSR_DR) == LSR_DR);
}

static void
uartNS16750_noread(struct io_chan *ops)
{
	(void)ops;
}

