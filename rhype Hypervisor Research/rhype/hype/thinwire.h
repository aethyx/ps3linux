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

#ifndef __THINWIRE_H_
#define __THINWIRE_H_

#include <config.h>
#include <types.h>
#include <io_chan.h>

extern void configThinWire(struct io_chan *ic);
extern void resetThinwire(void);
extern struct io_chan *getThinWireChannel(uval index);
extern const char thinwire_magic_string[14];

extern struct io_chan *(*serial_init_fn) (uval io_addr, uval32 clock,
					  uval32 baudrate);


#endif /* ! __THINWIRE_H_ */
