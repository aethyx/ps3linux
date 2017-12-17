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

#ifndef _TEST_H
#define _TEST_H

#include <config.h>
#include <asm.h>
#include <lpar.h>
#include <types.h>
#include <lib.h>
#include <hcall.h>
#include <partition.h>

/* these only exist for test OSes */
extern sval hprintf_nlk(const char *fmt, ...)
	__attribute__ ((format(printf, 1, 2), no_instrument_function));
extern sval vhprintf_nlk(const char *fmt, va_list ap)
	__attribute__ ((no_instrument_function));
extern void hputs_nlk(const char *buf)
	__attribute__ ((no_instrument_function));

#ifdef DEBUG
extern void ex_stack_assert(void) __attribute__ ((noreturn));
#endif


extern sval hcall_put_term_string(uval channel, uval count, const char *str);
extern sval hcall_get_term_string(int channel, char str[16]);
extern uval hcall_cons_init(uval ofd);

static inline void yield(uval arg){(void)arg;};



#endif /* _TEST_H */
