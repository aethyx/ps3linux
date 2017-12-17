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

#ifndef _OFH_H
#define _OFH_H

#include <config.h>
#include <types.h>
#include <of_devtree.h>

struct ofh_args_s {
	const char	*ofa_service;
	uval	ofa_nargs;
	uval	ofa_nreturns;
	sval	ofa_args[0];
};

typedef sval (ofh_func_t)(uval, uval, sval [], sval [], uval b);

struct ofh_srvc_s {
	const char	*ofs_name;
	ofh_func_t	*ofs_func;
	uval		ofs_hash;
};

extern ofh_func_t ofh_test_method;
extern ofh_func_t ofh_nosup;

/* device tree */
extern ofh_func_t ofh_peer;
extern ofh_func_t ofh_child;
extern ofh_func_t ofh_parent;
extern ofh_func_t ofh_instance_to_package;
extern ofh_func_t ofh_getproplen;
extern ofh_func_t ofh_getprop;
extern ofh_func_t ofh_nextprop;
extern ofh_func_t ofh_setprop;
extern ofh_func_t ofh_canon;
extern ofh_func_t ofh_finddevice;
extern ofh_func_t ofh_instance_to_path;
extern ofh_func_t ofh_package_to_path;
extern ofh_func_t ofh_call_method;

/* IO */
extern ofh_func_t ofh_open;
extern ofh_func_t ofh_close;
extern ofh_func_t ofh_read;
extern ofh_func_t ofh_write;
extern ofh_func_t ofh_seek;

/* memory */
extern ofh_func_t ofh_claim;
extern ofh_func_t ofh_release;

/* control */
extern ofh_func_t ofh_boot;
extern ofh_func_t ofh_enter;
extern ofh_func_t ofh_exit; /* __attribute__ ((noreturn)); */
extern ofh_func_t ofh_chain;

extern struct ofh_srvc_s ofh_srvc[];
extern struct ofh_srvc_s ofh_isa_srvc[];
extern sval ofh_active_package;

struct ofh_methods_s {
	const char *ofm_name;
	ofh_func_t *ofm_method;
};

struct ofh_ihandle_s {
	sval (*ofi_close)(void);
	sval (*ofi_read)(void *buf, uval count, uval *actual);
	sval (*ofi_write)(const void *buf, uval count, uval *actual);
	sval (*ofi_seek)(uval pos_hi, uval pos_lo, uval *status);
	struct ofh_methods_s *ofi_methods;
	sval ofi_node;
};

struct ofh_imem_s {
	sval (*ofi_xlate)(void *addr, uval ret[4]);
};


enum prop_type_e {
	pt_byte_array,
	pt_value,
	pt_string,
	pt_composite,
	/* these are for our own use */
	pt_func,
};
extern void ofh_cons_init(uval chan, uval b);
extern sval ofh_cons_read(void *buf, uval count, uval *actual);
extern sval ofh_cons_write(const void *buf, uval count, uval *actual);
extern sval ofh_cons_close(void);
extern sval ofh_handler(struct ofh_args_s *args, uval ifh_base);
extern sval leap(uval nargs, uval nrets, uval args[], uval rets[],
		 uval ba, uval f);

extern void ofh_vty_init(ofdn_t chosen, uval b);
extern void ofh_rtas_init(uval b);

extern void *_ofh_tree;
static inline void *
ofd_mem(uval base)
{
	return *DRELA(&_ofh_tree, base);
}

#endif /* _OFH_H */
