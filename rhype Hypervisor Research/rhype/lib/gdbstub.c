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
 * gdbstub: implements the architecture independant parts of the
 * gdb remote protocol.
 */

#include <config.h>
#include <types.h>
#include <lib.h>
#include <gdbstub.h>
#include <mmu.h>
#include <io_chan.h>

static uval active = 0;
static uval connected = 0;
static char in_buf[PGSIZE];
static char out_buf[PGSIZE];
static uval out_offset = 0;
static uval out_csum = 0;

static uval in_bytes = 0;	/* how much on in_buf is full */
static uval read_bytes = 0;	/* how much of in_buf has been read */

static uval out_acked = 0;

volatile struct cpu_state *current_state = NULL;


void
write_to_packet_hex(uval x, uval min_width)
{
	char buf[sizeof (uval) * 2 + 1];
	int i = sizeof (uval) * 2;
	buf[sizeof (uval) * 2] = 0;
	do {
		--i;
		buf[i] = hex2char(x & 15);
		x >>= 4;
	} while (x);
	while (i + min_width > sizeof (uval) * 2) {
		buf[--i] = '0';
	}
	write_to_packet(&buf[i]);
}


static uval __gdbstub
check_ack(void)
{
	sval z;
	char c;

	if (out_acked) {
		return 1;
	}

	do {
		z = gdb_io_read(&c, 1);
	} while (z != 1);

	switch (c) {
	case '+':
		return 1;
	case '-':
		return 0;
	default:
		hprintf("Bad ack: %c\n", c);
		return 0;
	}
}


static void __gdbstub
send_ack(void)
{
	gdb_io_write("+", 1);
}


static void __gdbstub
reset_out_buf(void)
{
	out_buf[0] = '$';
	out_buf[1] = 0;
	out_offset = 1;
	out_csum = 0;
	out_acked = 0;
}


void __gdbstub
write_to_packet(const char *str)
{
	while (*str) {
		out_buf[out_offset++] = *str;
		out_csum = (*str + out_csum) % 256;
		++str;
	}
	out_buf[out_offset] = 0;
}


static void __gdbstub
send_packet(void)
{
	out_buf[out_offset] = '#';
	out_buf[out_offset + 1] = hex2char(out_csum >> 4);
	out_buf[out_offset + 2] = hex2char(out_csum & 15);
	do {
		gdb_io_write(out_buf, out_offset + 3);
	} while (!check_ack());
}


static char *__gdbstub
read_packet(void)
{
	uval checksum = 0;
	sval x = 0;
	sval y;
	uval end = 0;
	uval start = 1;
retry:
	if (read_bytes) {
		memmove(&in_buf[0], &in_buf[read_bytes],
			in_bytes - read_bytes);
		in_bytes -= read_bytes;
		read_bytes = 0;
	}
	while (1) {
		if (read_bytes == in_bytes) {
			sval z;

			do {
				z = gdb_io_read(&in_buf[read_bytes],
						sizeof (in_buf) - read_bytes);
			} while (z <= 0);
			in_bytes += z;
		}
		if (read_bytes == 0 && in_buf[read_bytes] != '$') {
			if (in_buf[read_bytes] == '-') {
				send_packet();
				hprintf("gdb stub got back ack\n");
			} else if (in_buf[read_bytes] != '+') {
				hprintf("Non '$' packet header\n");
				goto screwed;
			} else {
				out_acked = 1;
			}
			++start;
		}

		if (in_buf[read_bytes] == '#') {
			end = 1;
		}

		if (read_bytes >= start && !end) {
			checksum = (checksum + in_buf[read_bytes]) % 256;
		}

		if (read_bytes >= 3 && in_buf[read_bytes - 2] == '#') {
			break;
		}
		++read_bytes;
	}
	/* compare checksum digits */
	x = checksum / 16;
	y = char2hex(in_buf[read_bytes - 1]);
	if (y != x)
		goto screwed;

	x = checksum % 16;
	y = char2hex(in_buf[read_bytes]);
	if (y != x)
		goto screwed;

	in_buf[read_bytes - 2] = 0;

	/* consume the last character processed */
	++read_bytes;
	return &in_buf[start];
screwed:

	gdb_io_write("-", 1);
	/* just keep trying next packet... */
	goto retry;
}


struct cpu_state *__gdbstub
enter_gdb(struct cpu_state *state, uval exception_type)
{
	uval signum;

	gdb_enter_notify(exception_type);

	if (active) {
		hprintf("double entry of gdb stub\n");
		while (1) ;
	}
	active = 1;

	if (!connected) {
		hprintf("GDB connection activated\n");
		gdb_print_state(state);
		connected = 1;
	}

	signum = gdb_signal_num(state, exception_type);

	current_state = state;

	/* XXX if gdb tries attaching before this, we go into
	 * a loop and never get synced with it.
	 */
	reset_out_buf();
	write_to_packet("S");
	write_to_packet_hex(signum, 2);
	send_packet();

	while (1) {
		char *packet = read_packet();

		send_ack();
		reset_out_buf();
		switch (packet[0]) {
		case '?':	/* query signal number */
			write_to_packet("S");
			write_to_packet_hex(signum, 2);
			send_packet();
			break;
		case 'H':	/* thread operations */
			write_to_packet("OK");
			send_packet();
			break;
		case 'g':	/* get register state */
			gdb_write_to_packet_reg_array(state);
			send_packet();
			break;
		case 'G':	/* set register state */
			gdb_read_reg_array(state, packet + 1);
			write_to_packet("OK");
			send_packet();
			break;
		case 'm':{	/* read from memory */
			uval i = 1;
			uval len;
			sval ret;
			char *addr;
			char *lenstr;

			while (packet[i] != ',')
				++i;
			packet[i] = 0;
			addr = (char *)str2uval(packet + 1, sizeof (uval));
			++i;

			lenstr = packet + i;
			len = str2uval(lenstr, sizeof (uval));

			ret = gdb_write_to_packet_mem(state, addr, len);
			if (len != (uval)ret) {
				reset_out_buf();
				/* don't know what error to use */
				write_to_packet("E11");
			}

			send_packet();
			break;
		}
		case 'M':{	/* write to memory */
			uval i = 1;
			uval len;
			char *addr;
			char *lenstr;

			while (packet[i] != ',')
				++i;
			packet[i] = 0;
			addr = (char *)str2uval(packet + 1, sizeof (uval));
			++i;

			lenstr = packet + i;
			while (packet[i] != ':')
				++i;
			packet[i] = 0;
			len = str2uval(lenstr, sizeof (uval));
			++i;

			i = gdb_write_mem(state, addr, len, packet + i);
			if (i == len) {
				write_to_packet("OK");
			} else {
				write_to_packet("E11");
			}

			send_packet();
			break;
		}
		case 'D':	/* detach */
			write_to_packet("OK");
			send_packet();
			/* fall through */
		case 'k':	/* kill, do not repond */
			connected = 0;
			/* fall through */
		case 's':	/* step */
		case 'c':{	/* continue */
			uval addr = ~((uval)0);
			uval type = GDB_CONTINUE;

			if (packet[0] == 's') {
				type = GDB_STEP;
			}
			if (packet[1]) {
				addr = str2uval(packet + 1,
						sizeof (uval));
			}

			state = gdb_resume(state, addr, type);
			goto done;
		}
		default:
			write_to_packet("");
			send_packet();
			break;
		}
	}
done:
	active = 0;
	return state;
}

static uval __gdbstub
_lib_gdb_xlate_ea(uval eaddr)
{
	return eaddr;
}
extern uval __gdbstub gdb_xlate_ea(uval eaddr)
	__attribute__ ((weak, alias("_lib_gdb_xlate_ea")));
