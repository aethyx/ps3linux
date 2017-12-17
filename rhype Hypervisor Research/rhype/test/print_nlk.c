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
#include <test.h>

extern struct io_chan *print_out;

/*
 * hprintf_nlk bypasses the console lock (if possible; it currently
 * isn't in thinwire's case). As such it is perfect for debug output
 * while the console lock is held (by a kernel currently writing via
 * an hcall).
 */

sval
hprintf_nlk(const char *fmt, ...)
{
	uval sz;
	va_list ap;

	int l =	lock_tryacquire(&print_out->lock);

	va_start(ap, fmt);
	sz = _vhprintf(fmt, ap, 0);
	va_end(ap);

	if (l) {
		lock_release(&print_out->lock);
	}

	return sz;
}

void
hputs_nlk(const char *buf)
{
	if (print_out == NULL) {
		return;
	}

	int l =	lock_tryacquire(&print_out->lock);

	uval len = strlen(buf);
	print_out->ic_write(print_out, buf, len);

	if (l) {
		lock_release(&print_out->lock);
	}
}

sval
vhprintf_nlk(const char *fmt, va_list ap)
{
	sval ret;
	int l =	lock_tryacquire(&print_out->lock);
	ret =  _vhprintf(fmt, ap, 0);
	if (l) {
		lock_release(&print_out->lock);
	}
	return ret;
}


void
assprint(const char *expr, const char *file, int line, const char *fmt, ...)
{
	va_list ap;

	/* I like compiler like output */
	hprintf("%s:%d: ASSERT(%s)\n", file, line, expr);

	va_start(ap, fmt);
	vhprintf_nlk(fmt, ap);
	va_end(ap);
	hputs("\n");

	breakpoint();
}
