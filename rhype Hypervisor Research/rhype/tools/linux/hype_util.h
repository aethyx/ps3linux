/*
 * Copyright (C) 2005 Michal Ostrowski <mostrows@watson.ibm.com>, IBM Corp.
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
 * Hype definitions for user applications, utility functions.
 *
 */
#ifndef HYPE_UTIL_H
#define HYPE_UTIL_H

#include <hype_types.h>
#ifdef UVAL_IS_64
#define UVAL_CHOOSE(x32, x64) x64
#else
#define UVAL_CHOOSE(x32, x64) x32
#endif

#define HYPE_ROOT "/var/hype"

#define ALIGN_UP(a,s)	(((a) + ((s) - 1)) & (~((s) - 1)))
#define ALIGN_DOWN(a,s)	((a) & (~((s) - 1)))

#define ASSERT(cond, args...)					\
	if (!(cond)) {						\
		printf("ASSERT: %d\n", __LINE__);		\
		printf(args); exit(0);				\
	}


extern int hcall_init(void); /* returns /dev/hcall fd */
extern int hcall(oh_hcall_args* hargs);
extern int sig_xirr_bind(uval32 xirr, int sig);
extern int parse_laddr(const char* arg, uval *base, uval *size);
extern uval mem_hold(uval size);

extern int get_file(const char* name, char* buf, int len);
extern int get_file_numeric(const char* name, uval *val);
extern int set_file(const char* name, const char* buf, int len);
extern int set_file_printf(const char* name, const char* fmt, ...);
extern int of_make_node(const char* name);
extern int of_set_prop(const char* node, const char* prop,
		       const void* data, int len);

extern void oh_set_pname(const char* pname);
extern const char* oh_pname;

#endif /* HYPE_UTIL_H */
