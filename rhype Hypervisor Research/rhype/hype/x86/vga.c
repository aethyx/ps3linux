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
 * VGA console driver
 *
 */

#include <config.h>
#include <lib.h>

/*
 * Base for default 80x25 text screen,
 * every character on screen uses 2 bytes
 *		- the first for the ASCII code and
 *		- the 2nd byte for special modes (like reverse video, ..)
 */
unsigned char *vga_base;

enum { N_ROWS = 25, N_COLS = 80 };
static int row;
static int col;

static sval vga_write(struct io_chan *chan, const char *buf, uval len);

/*
 * io_chan structure for vga
 */
struct io_chan vga_chan = {
	.ic_write = vga_write,
};

/* proto types */
void cls(void);
void scroll_up(void);

/* implementation */
void
cls(void)
{
	uval i;

	for (i = 0; i < N_ROWS * N_COLS; i++) {
		*(vga_base + i * 2) = ' ';
	}
}

void
set_vga_base(uval base)
{
	vga_base = (unsigned char *)base;
}

struct io_chan *
vga_init(uval baseaddr)
{
	vga_base = (unsigned char *)baseaddr;
	row = col = 0;
	cls();
	return fill_io_chan(&vga_chan);
}

void
scroll_up(void)
{
	int i;

	for (i = 0; i < N_ROWS - 1; i++) {
		memmove(vga_base + i * N_COLS * 2,
			vga_base + (i + 1) * N_COLS * 2, N_COLS * 2);
	}
}

static sval
vga_write(struct io_chan *chan, const char *buf, uval len)
{
	(void)chan;
	uval l = len;

	while (l--) {
		char c = *buf++;

		if (c == '\r') {
			/* ignore \r */
			continue;
		}
		if (c == '\n') {
			row++;
			col = 0;
		} else {
			*(vga_base + row * (N_COLS * 2) + col * 2) = c;
			col++;
		}
		if (col >= N_COLS - 1) {
			col = 0;
			row++;
		}
		if (row >= N_ROWS - 1) {
			col = 0;
			row--;
			scroll_up();
		}
	}
	return len;
}
