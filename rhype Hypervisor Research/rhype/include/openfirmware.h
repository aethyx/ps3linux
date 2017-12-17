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

#ifndef _OPENFIRMWARE_H
#define _OPENFIRMWARE_H

#define OF_SUCCESS 0
#define OF_FAILURE -1

typedef sval32 phandle;
typedef sval32 ihandle;

#define CI_NARGS 8

struct service_request {
	uval32 service;
	uval32 n_args;
	uval32 n_returns;
	uval32 args[CI_NARGS];
};

extern sval32 prom_call(void *arg, uval rtas_base, uval func, uval msr);

extern void of_init(uval ci_addr, uval of_msr) __attribute__ ((weak));

extern sval32 call_of(const char *service_name, uval32 nargs, uval32 nrets,
		      uval32 rets[], ...);

extern sval32 of_finddevice(const char *devspec);
extern sval32 of_getprop(phandle ph, const char *name,
			 void *buf, uval32 buflen);
extern sval32 of_getproplen(phandle ph, const char *name);
extern phandle of_getparent(phandle ph);
extern phandle of_getpeer(phandle ph);
extern phandle of_getchild(phandle ph);
extern sval32 of_write(ihandle ih, const char *addr, uval32 len);
extern sval32 of_read(ihandle ih, char *addr, uval32 len);
extern sval32 of_instance_to_path(ihandle ih, char *buffer, uval32 len);
extern sval32 of_instance_to_package(ihandle ih);
extern sval32 of_package_to_path(phandle ph, char *buffer, uval32 buflen);
extern sval of_devtree_save(uval of_mem, uval of_sz);


#endif /* ! _OPENFIRMWARE_H */
