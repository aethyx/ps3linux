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

#ifndef _VTTY_H
#define _VTTY_H

#include <config.h>
#include <types.h>
#include <io_chan.h>

extern sval vtty_put_term_char16(struct cpu_thread *thread, uval channel,
				 uval count, const char *data);
extern sval vtty_get_term_char16(struct cpu_thread *thread, uval channel,
				 char *data);

extern void vtty_init(struct os *os);
extern sval vtty_term_init(struct os *os, uval idx, struct io_chan *chan);

#endif /* ! _VTTY_H */
