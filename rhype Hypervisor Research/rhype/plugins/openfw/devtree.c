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
 *
 */

#include <ofh.h>
#include <lib.h>
#include <of_devtree.h> 
#include <util.h>

sval
ofh_peer(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	if (nargs == 1) {
		if (nrets == 1) {
			ofdn_t ph = argp[0];
			uval *sib_ph = &retp[0];
			void *mem = ofd_mem(b);

			*sib_ph = ofd_node_peer(mem, ph);
			return OF_SUCCESS;
		}	
	}
	return OF_FAILURE;
}	

sval
ofh_child(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	if (nargs == 1) {
		if (nrets == 1) {
			ofdn_t ph = argp[0];
			uval *ch_ph = &retp[0];
			void *mem = ofd_mem(b);

			*ch_ph = ofd_node_child(mem, ph);
			return OF_SUCCESS;
		}	
	}
	return OF_FAILURE;
}	

sval
ofh_parent(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	if (nargs == 1) {
		if (nrets == 1) {
			ofdn_t ph = argp[0];
			uval *parent_ph = &retp[0];
			void *mem = ofd_mem(b);

			*parent_ph = ofd_node_parent(mem, ph);
			return OF_SUCCESS;
		}	
	}
	return OF_FAILURE;
}	

sval
ofh_instance_to_package(uval nargs, uval nrets, sval argp[], sval retp[],
			uval b __attribute__ ((unused)))
{
	if (nargs == 1) {
		if (nrets == 1) {
			struct ofh_ihandle_s *ih =
				(struct ofh_ihandle_s *) argp[0];
			uval *p = &retp[0];

			*p = (sval)ih->ofi_node;
			return OF_SUCCESS;
		}
	}
	return OF_FAILURE;
}

sval
ofh_getproplen(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	if (nargs == 2) {
		if (nrets == 1) {
			ofdn_t ph = argp[0];
			const char *name = (const char *)argp[1];
			sval *size = &retp[0];
			void *mem = ofd_mem(b);

			*size = ofd_getproplen(mem, ph, name);
			if (*size >= 0) {
				return OF_SUCCESS;
			}
		}
	}
	return OF_FAILURE;
}

sval
ofh_getprop(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	if (nargs == 4) {
		if (nrets == 1) {
			ofdn_t ph = argp[0];
			const char *name = (const char *)argp[1];
			void *buf = (void *)argp[2];
			uval buflen = argp[3];
			sval *size = &retp[0];
			void *mem = ofd_mem(b);

			*size = ofd_getprop(mem, ph, name, buf, buflen);
			if (*size > 0) {
				return OF_SUCCESS;
			}
		}
	}
	return OF_FAILURE;
}

sval
ofh_nextprop(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	if (nargs == 3) {
		if (nrets == 1) {
			ofdn_t ph = argp[0];
			const char *prev = (const char *)argp[1];
			char *name = (char *)argp[2];
			sval *flag = &retp[0];
			void *mem = ofd_mem(b);

			*flag = ofd_nextprop(mem, ph, prev, name);
			return OF_SUCCESS;
		}
	}
	return OF_FAILURE;
}

sval
ofh_setprop(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	if (nargs == 4) {
		if (nrets == 1) {
			ofdn_t ph = argp[0];
			const char *name = (const char *)argp[1];
			const void *buf = (void *)argp[2];
			uval buflen = argp[3];
			sval *size = &retp[0];
			void *mem = ofd_mem(b);

			*size = ofd_setprop(mem, ph, name, buf, buflen);
			return OF_SUCCESS;
		}
	}
	return OF_FAILURE;
}

sval
ofh_canon(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	if (nargs == 3) {
		if (nrets == 1) {
			const char *dev_spec = (const char *)argp[0];
			char *buf = (char *)argp[1];
			uval sz = argp[2];
			uval *len = &retp[0];
			void *mem = ofd_mem(b);
			ofdn_t ph;

			ph = ofd_node_find(mem, dev_spec);
			if (ph > 0) {
				*len = ofd_node_to_path(mem, ph, buf, sz);
				return OF_SUCCESS;
			}
		}
	}
	return OF_FAILURE;
}

sval ofh_active_package = -1;

sval
ofh_finddevice(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	if (nargs == 1) {
		if (nrets == 1) {
			sval *ap = DRELA(&ofh_active_package, b);
			const char *devspec = (const char *)argp[0];
			sval *ph = &retp[0];
			void *mem = ofd_mem(b);

			/* good enuff */
			if (devspec[0] == '\0') {
				if (*ap == -1) {
					return OF_FAILURE;
				}
				*ph = *ap;
			} else {
				*ph = ofd_node_find(mem, devspec);
				if (*ph <= 0) {
					return OF_FAILURE;
				}
			}
			*ap = *ph;
			return OF_SUCCESS;
		}
	}
	return OF_FAILURE;
}
		
sval
ofh_instance_to_path(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	if (nargs == 3) {
		if (nrets == 1) {
			struct ofh_ihandle_s *ih =
				(struct ofh_ihandle_s *)argp[0];
			char *buf = (char *)argp[1];
			uval sz = argp[2];
			uval *len = &retp[0];
			ofdn_t ph;
			void *mem = ofd_mem(b);

			ph = ih->ofi_node;
			*len = ofd_node_to_path(mem, ph, buf, sz);

			return OF_SUCCESS;
		}
	}
	return OF_FAILURE;
}

sval
ofh_package_to_path(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	if (nargs == 3) {
		if (nrets == 1) {
			ofdn_t ph = argp[0];
			char *buf = (char *)argp[1];
			uval sz = argp[2];
			sval *len = &retp[0];
			void *mem = ofd_mem(b);

			*len = ofd_node_to_path(mem, ph, buf, sz);

			return OF_SUCCESS;
		}
	}
	return OF_FAILURE;
}



