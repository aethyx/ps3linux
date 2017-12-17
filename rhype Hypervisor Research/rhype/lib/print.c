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
#include <types.h>
#include <lib.h>
#include <stdarg.h>

struct io_chan *print_out = NULL;

/*
 * Initialize printing, indicating how the output is to be
 * processed.  It is legal to initialize multiple times, if
 * printing needs to be redirected at various times during
 * boot or some such.
 */

struct io_chan *
hout_set(struct io_chan *out)
{
	struct io_chan *old = print_out;
	print_out = out;
	return old;
}

struct io_chan *
hout_get(void)
{
	return print_out;
}

void
hputs(const char *buf)
{
	if (print_out != NULL) {
		uval len = strlen(buf);
		lock_acquire(&print_out->lock);
		print_out->ic_write(print_out, buf, len);
		lock_release(&print_out->lock);
	}
}

#define PRINT_BUFSZ 256

sval
_vhprintf(const char *fmt, va_list ap, int lk)
{
	const char trunc[] = "(TRUNCATED)\n";
	char hbuf[PRINT_BUFSZ];
	uval sz;

	if (print_out == NULL)
		return -1;

	sz = vsnprintf(hbuf, sizeof (hbuf), fmt, ap);

	if (lk) {
		lock_acquire(&print_out->lock);
	}

	if (sz < sizeof (hbuf)) {
		print_out->ic_write(print_out, hbuf, sz);
	} else {
		print_out->ic_write(print_out, hbuf, sizeof (hbuf));
		print_out->ic_write(print_out, trunc, sizeof (trunc));
		sz = sizeof (hbuf);
	}

	if (lk)
		lock_release(&print_out->lock);

	return sz;
}

sval
vhprintf(const char *fmt, va_list ap)
{
	return _vhprintf(fmt, ap, 1);
}

sval
hprintf(const char *fmt, ...)
{
	sval sz;
	va_list ap;

	va_start(ap, fmt);
	sz = _vhprintf(fmt, ap, 1);
	va_end(ap);

	return sz;
}

#include <lpar.h>
static const char *_hcall_error_list[] = {
	[-1 * H_Hardware]	= "Hardware Error",
	[-1 * H_Function]	= "Not Supported",
	[-1 * H_Privilege]	= "Caller not in privileged mode",
	[-1 * H_Parameter]	= "Outside Valid Range for Partition",
	[-1 * H_Bad_Mode]	= "Illegal or conflicting MSR value",
	[-1 * H_PTEG_FULL]	= "The requested pteg was full",
	[-1 * H_NOT_FOUND]	= "The requested pte was not found",
	[-1 * H_RESERVED_DABR]	= "Address is reserved by the Hypervisor",
	[-1 * H_UNAVAIL]	= "Requested resource unavailable"
};

const char *
hstrerror(sval herr)
{
	static const uval sz = sizeof (_hcall_error_list) /
		sizeof (_hcall_error_list[1]);
	if (herr < 0) {
		uval err = -1 * herr;
		if (err < sz) {
			return _hcall_error_list[err];
		}
	}
	return NULL;
}
