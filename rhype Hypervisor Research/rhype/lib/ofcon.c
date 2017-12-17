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
#include <lib.h>
#include <openfirmware.h>

static phandle pkg;
static ihandle of_stdout;

#if 0
static void
ofcon_close(void)
{
//	uval32 rets[4];
	memset(&ofcon_ic, 0, sizeof(ofcon_ic));
//	call_of("close", 1, 1, rets, of_stdout);
}
#endif

static sval
ofcon_write(struct io_chan *ops, const char *addr, uval len)
{
	(void)ops;
#if 1
	return of_write(of_stdout, addr, len);
#else
	uval i;
	uval x = 0;
	for (i = 0; i < len ; i++) {
		if (addr[i] == '\n') {
			of_write(of_stdout, addr+x, i-x);
			of_write(of_stdout, "\r",1);
			x=i;
		}
	}
	if (i-x) {
		of_write(of_stdout, addr+x, i-x);
	}
	return (sval) len;
#endif
}

static sval
ofcon_write_avail(struct io_chan *ops)
{
	(void) ops;
	of_write(of_stdout, "ofcon_write_avail called", 24);
	return 2048;
}

static sval
ofcon_read_all(struct io_chan *ops __attribute__ ((unused)),
	       char *buf, uval len)
{
	sval l = len;
	while (l > 0) {
		sval sz;

		sz = of_read(of_stdout, buf, len);
		if (sz != -1) {
			buf += sz;
			l -= sz;
		}
	}
	return len;
}

static sval
ofcon_read(struct io_chan *ops __attribute__ ((unused)),
	   char *buf, uval len)
{
	return of_read(of_stdout, buf, len);
}

static sval
ofcon_read_avail(struct io_chan *ops __attribute__ ((unused)))
{
	return 1024;
}

static void
ofcon_noread(struct io_chan *ops __attribute__ ((unused)))
{
	return;
}

/*
 * io_chan structure for open firmware console.
 */
struct io_chan ofcon_ic = {
	.ic_write = ofcon_write,
	.ic_write_avail = ofcon_write_avail,
	.ic_read =	ofcon_read,
	.ic_read_all =	ofcon_read_all,
	.ic_read_avail = ofcon_read_avail,
	.ic_noread =	ofcon_noread
};

struct io_chan *
ofcon_init(void)
{
	const char initmsg[] = "Earliest possible OF output\r\n";
	pkg = of_finddevice("/chosen");

	of_getprop(pkg, "stdout", &of_stdout, sizeof (of_stdout));

	of_write(of_stdout, initmsg, sizeof(initmsg));

#if OFCON_DEBUG
	{
		uval msr;
		char buffer[512];
		char buffer2[512];
		sval32 rets[1];

		phandle stdout_ph;

		sprintf(buffer,"ofcon_init, pkg=%x, stdout=%x\r\n",
			pkg, of_stdout);
		of_write(of_stdout, buffer, strlen(buffer));

		call_of("canon",3,1,rets, "/chosen", buffer2, 32);
		sprintf(buffer, "canonical path for /chosen >>%s<< \r\n",
			buffer2);
		of_write(of_stdout, buffer, strlen(buffer));

		sprintf(buffer,"<nil>");
		of_instance_to_path(of_stdout,buffer2,sizeof(buffer2));


		snprintf(buffer, sizeof(buffer) -1,
			 "instance path >>%s<<\r\n", buffer2);
		of_write(of_stdout, buffer, strlen(buffer));

		stdout_ph = (phandle) of_instance_to_package(of_stdout);
		sprintf(buffer, "package for stdout=%x\r\n", stdout_ph);
		of_write(of_stdout, buffer, strlen(buffer));

		of_package_to_path(stdout_ph, buffer2, sizeof(buffer2));
		snprintf(buffer, sizeof(buffer),"package path >>%s<< \r\n",
			 buffer2);
		of_write(of_stdout, buffer, strlen(buffer));

		asm ("mfmsr %0": "=r"(msr));

		sprintf(buffer, "msr = %16lx\r\n", msr);
		of_write(of_stdout, buffer, strlen(buffer));
	}
#endif
	return &ofcon_ic;
}

