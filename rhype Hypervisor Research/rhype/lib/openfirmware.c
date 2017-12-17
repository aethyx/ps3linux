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
 * CHRP Open Firmware interface
 */

#include <config.h>
#include <stdarg.h>	/* va_args */
#include <openfirmware.h>
#include <lib.h>
struct pci_addr {
	uval32 phys_hi, phys_mid, phys_lo;
};

struct isa_addr {
	uval32 phys_hi, phys_lo;
};

/* OF client interface address, passed in r5 at boot */


static uval client_interface;
static uval of_msr;

uval of_debug;
#define debug(args...) \
	if (of_debug) hprintf(args);

void
of_init(uval ci_addr, uval orig_of_msr)
{
	client_interface = ci_addr;
	of_msr = orig_of_msr;
}

/*
 * Note: Make sure all return codes and values are preset to -1, because if OF
 * aborts due to error it frequently will not touch those values itself.
 */

/* call_of()
 * returns -1 for failure, 0 for success
 *	service_name - NULL-terminated string specifying OF service
 *	nargs - number of values in rets[] which are input params
 *	nrets - number of values in rets[] (beginning *after* nargs) which are
 *			output params
 *	rets - array into which output params are copied
 *	... - arguments to the OF service
 */

sval32
call_of(const char *service_name, uval32 nargs, uval32 nrets,
	uval32 rets[], ...)
{
	struct service_request req;
	va_list args;
	uval32 i;
	sval32 retcode = -1;
	if (client_interface == 0) {
		return -1;
	}

	memset(&req, 0, sizeof(req));
	req.service = (uval)service_name;
	req.n_args = nargs;
	req.n_returns = nrets;

	/* copy all the params into the args array */
	va_start(args, rets);
	for (i = 0; i < nargs; i++) {
		req.args[i] = va_arg(args, uval);
	}
	va_end(args);

	/* call OF client interface */
	/* a direct function call is not possible since */
	/* openfirmware wants to run in 32bit mode      */

	retcode = prom_call(&req, 0, client_interface, of_msr);

	/* copy from the end of our args into rets[] */
	for (i=0; i < nrets; i++) {
		rets[i] = req.args[i+nargs];
	}
	if (retcode != -1) retcode = 0;
	return retcode;
}

/* convenience functions */


sval32
of_finddevice(const char *devspec)
{
	sval32 rets[1] = {-1};
	call_of("finddevice", 1, 1, rets, devspec);
	if (rets[0] == -1) {
		debug("finddevice %s -> FAILURE %d\n",devspec,rets[0]);
		return -1;
	}
	debug("finddevice %s -> %d\n",devspec, rets[0]);
	return rets[0];
}


sval32
of_instance_to_path(ihandle ih, char *buffer, uval32 buflen)
{
	sval32 rets[1] = { -1};
	if (-1 == call_of("instance-to-path", 3, 1, rets, ih, buffer, buflen))
		return -1;
	return rets[0];

}

sval32
of_getprop(phandle ph, const char *name, void *buf, uval32 buflen)
{
	sval32 rets[1] = {-1};
	call_of("getprop", 4, 1, rets, ph, name, buf, buflen);
	if (-1 ==  rets[0]) {
		debug("getprop %x %s -> FAILURE\n", ph, name);
		return -1;
	}
	debug("getprop %x %s -> %s\n", ph, name, (char*)buf);
	return rets[0];
}

sval32
of_getproplen(phandle ph, const char *name)
{
	sval32 rets[1] = {-1};
	call_of("getproplen", 2, 1, rets, ph, name);
	if (-1 == rets[0]) {
		debug("getproplen %x %s -> FAILURE\n", ph, name);
		return -1;
	}
	debug("getproplen %x %s -> 0x%x\n", ph, name, rets[0]);
	return rets[0];
}

sval32
of_write(ihandle ih, const char *addr, uval32 len)
{
	sval32 rets[1] = {-1};
	uval old = of_debug;
	of_debug = 0;
	if (-1 == call_of("write", 3, 1, rets, ih, addr, len)) {
		of_debug = old;
		return -1;
	}
	of_debug = old;
	return rets[0];
}

sval32
of_read(ihandle ih, char *addr, uval32 len)
{
	sval32 rets[1] = {-1};
	call_of("read", 3, 1, rets, ih, addr, len);
	if (-1 == rets[0])
		return -1;
	return rets[0];
}

sval32
of_instance_to_package(ihandle ih)
{
	sval32 rets[1] = { -1};
	call_of("instance-to-package", 1, 1, rets, ih);
	if (-1 == rets[0])
		return -1;
	return rets[0];

}

sval32
of_package_to_path(phandle ph, char *buffer, uval32 buflen)
{
	sval32 rets[1] = { -1};
	call_of("package-to-path", 3, 1, rets, ph, buffer, buflen);
	if (-1 == rets[0])
		return -1;
	return rets[0];
}

phandle
of_getparent(phandle ph)
{
	sval32 rets[1] = { OF_FAILURE };
	call_of("parent", 1, 1, rets, ph);
	debug("getparent %x -> %x\n", ph, rets[0]);
	return rets[0];
}

phandle
of_getchild(phandle ph)
{
	sval32 rets[1] = { OF_FAILURE };
	call_of("child", 1, 1, rets, ph);
	debug("getchild %x -> %x\n", ph, rets[0]);
	return rets[0];
}

phandle
of_getpeer(phandle ph)
{
	sval32 rets[1] = { OF_FAILURE };
	call_of("peer", 1, 1, rets, ph);
	debug("getpeer %x -> %x\n", ph, rets[0]);
	return rets[0];
}
