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
#include <util.h>

sval
ofh_open(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	if (nargs == 1) {
		if (nrets == 1) {
			const char *devspec = (const char *)argp[0];
			uval *ih = &retp[0];
			ofdn_t p;
			void *mem = ofd_mem(b);

			p = ofd_node_find(mem, devspec);
			if (p > 0) {
				ofdn_t io;
				io = ofd_node_io(mem, p);
				if (io > 0) {
					uval f = ofd_io_open(mem, io);
					if (f != 0) {
						*ih = leap(b, 0, NULL, NULL,
							   b, (uval)f);
						return OF_SUCCESS;
					}
				}
			}
			*ih = 0;
		}
	}
	return OF_FAILURE;
}

sval
ofh_close(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	if (nargs == 1) {
		if (nrets == 0) {
			argp = argp;
			retp = retp;
			b = b;
			return  OF_FAILURE;
		}
	}
	return OF_FAILURE;
}
sval
ofh_read(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	if (nargs == 3) {
		if (nrets == 1) {
			struct ofh_ihandle_s *ih =
				(struct ofh_ihandle_s *)argp[0];

			if (ih->ofi_read != NULL) {
				uval addr = argp[1];
				uval sz = argp[2];
				uval *actual = &retp[0];
				uval f = (uval)ih->ofi_read;

				if (f != 0) {
					return leap(addr, sz, actual, NULL,
						    b, f);
				}
			}
		}
	}
	return OF_FAILURE;
}

sval
ofh_write(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	if (nargs == 3) {
		if (nrets == 1) {
			struct ofh_ihandle_s *ih =
				(struct ofh_ihandle_s *)argp[0];

			if (ih->ofi_write != NULL) {
				uval addr = argp[1];
				uval sz = argp[2];
				uval *actual = &retp[0];
				uval f = (uval)ih->ofi_write;

				if (f != 0) {
					return leap(addr, sz, actual, NULL,
						    b, f);
				}
			}
		}
	}
	return OF_FAILURE;
}

sval
ofh_seek(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	b=b;
	nargs = nargs;
	nrets = nrets;
	argp = argp;
	retp = retp;
	return OF_FAILURE;
}

static ofh_func_t *
method_lookup(struct ofh_ihandle_s *ih, const char *name, uval b)
{
	struct ofh_methods_s *m = DRELA(ih->ofi_methods, b);

	while (m != NULL && m->ofm_name != NULL ) {
		if (strcmp(name, DRELA(m->ofm_name, b)) == 0) {
			return m->ofm_method;
		}
	}
	return NULL;
}


sval
ofh_call_method(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	if (nargs > 2) {
		if (nrets > 1) {
			const char *method = (const char *)argp[0];
			struct ofh_ihandle_s *ih =
				(struct ofh_ihandle_s *)argp[1];
			ofh_func_t *f;

			f = method_lookup(ih, method, b);
			if (f != NULL) {
				/* set catch methods return 0 on success */
				retp[0] = leap(nargs - 2, nrets - 1,
					     &argp[2], &retp[1], b, (uval)f);
				return OF_SUCCESS;
			}
		}
	}				   
	return OF_FAILURE;
}

