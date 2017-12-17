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

#ifndef _VTERM_H
#define _VTERM_H

#include <config.h>
#include <types.h>
#include <cpu_thread.h>
#include <os.h>

extern uval vt_sys_init(void);
extern sval vt_char_put(struct cpu_thread *thread, uval chan,
			uval c, const char buf[16]);
extern sval vt_char_get(struct cpu_thread *thread, uval chan, char buf[16]);
extern sval vts_register(struct os *os, uval ua, uval lpid, uval pua);
extern sval vts_free(struct os *os, uval ua);
extern sval vts_partner_info(struct os *os, uval ua, uval plpid,
			     uval pua, uval lba);

#endif /* ! _VTERM_H */
