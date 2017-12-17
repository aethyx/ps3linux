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
#include <rtas.h>
#include <openfirmware.h>
#include <lib.h>

/*
 * Some bringup firmware has created an rtas console abstraction so
 * that client OSes do not have write custom drivers for buggy UARTS
 * on bringup boards.
 */

static sval32 putc_token = -1;
static sval32 getc_token = -1;

static sval
ioc_rtas_putc(uval8 c)
{
	struct rtas_args_s *r;
	sval ret;

	assert(putc_token > 0, "RAS Console was never inited\n");

	r = alloca(sizeof (*r) + sizeof (r->ra_args[0]) * 3);
	r->ra_token = putc_token;
	r->ra_nargs = 1;
	r->ra_nrets = 1;
	r->ra_args[0] = c;
	ret = rtas_call(r);
	if ( /* ret == RTAS_SUCCEED && */ r->ra_args[1] == RTAS_SUCCEED) {
		return c;
	}
	return -1;
}
static sval
ioc_rtas_getc(void)
{
	struct rtas_args_s *r;
	sval ret;

	assert(getc_token > 0, "RAS Console was never inited\n");

	r = alloca(sizeof (*r) + sizeof (r->ra_args[0]) * 4);
	r->ra_token = getc_token;
	r->ra_nargs = 0;
	r->ra_nrets = 2;
	ret = rtas_call(r);
	if ( /* ret == RTAS_SUCCEED && */ r->ra_args[0] == RTAS_SUCCEED) {
		return r->ra_args[1];
	}
	return -1;
}

static sval
ioc_rtas_write(struct io_chan *ops __attribute__ ((unused)),
	       const char *buf, uval len)
{
	sval l = len;

	while (l > 0) {
		if (ioc_rtas_putc(*buf) != -1) {
			++buf;
			--l;
		}
	}
	return len;
}

static sval
ioc_rtas_write_avail(struct io_chan *ops __attribute__ ((unused)))
{
	assert(putc_token > 0, "RAS Console was never inited\n");
	/* some large value */
	return 1024;
}

static sval
ioc_rtas_read_all(struct io_chan *ops __attribute__ ((unused)),
		  char *buf, uval len)
{
	sval l = len;
	sval c;

	while (l > 0) {
		c = ioc_rtas_getc();
		if (c != -1) {
			*buf = c;
			++buf;
			--l;
		}
	}
	return len;
}

static sval
ioc_rtas_read(struct io_chan *ops __attribute__ ((unused)),
	      char *buf, uval len)
{
	sval l = len;
	sval c;

	while (l > 0) {
		c = ioc_rtas_getc();
		if (c == -1) {
			break;
		}
		*buf = c;
		++buf;
		--l;
	}
	return len - l;
}

static sval
ioc_rtas_read_avail(struct io_chan *ops __attribute__ ((unused)))
{
	assert(getc_token > 0, "RAS Console was never inited\n");
	/* some large value */
	return 1;
}

static void
ioc_rtas_noread(struct io_chan *ops __attribute__ ((unused)))
{
	assert(getc_token > 0, "RAS Console was never inited\n");
}

struct io_chan ioc_rtas = {
	.ic_write = ioc_rtas_write,
	.ic_write_avail = ioc_rtas_write_avail,
	.ic_read = ioc_rtas_read,
	.ic_read_all = ioc_rtas_read_all,
	.ic_read_avail = ioc_rtas_read_avail,
	.ic_noread = ioc_rtas_noread
};

struct io_chan *
io_chan_rtas_init(void)
{
	phandle rtas;
	sval ret;
	static const char putc[] = "put-term-char";
	static const char getc[] = "get-term-char";

	if (rtas_entry == 0) {
		return NULL;
	}
	/*
	 * check to see if standard PCI RTAS calls are supported
	 */
	rtas = of_finddevice("/rtas");
	if (rtas <= 0) {
#ifdef DEBUG
		hprintf("%s: no RTAS so no RTAS io_chan\n", __func__);
#endif
		return NULL;
	}

	ret = of_getprop(rtas, putc, &putc_token, sizeof (putc_token));
	if (ret == -1) {
#ifdef DEBUG
		hprintf("%s: no \"%s\" do no RTAS IO Chan\n", __func__, putc);
#endif
		return NULL;
	}
	ret = of_getprop(rtas, getc, &getc_token, sizeof (getc_token));
	if (ret == -1) {
		putc_token = getc_token;
#ifdef DEBUG
		hprintf("%s: no \"%s\" do no RTAS IO Chan\n", __func__, getc);
#endif
		return NULL;
	}

	return &ioc_rtas;
}
