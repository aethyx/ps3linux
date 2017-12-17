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

#include <config.h>
#include <lib.h>
#include <io.h>

void
breakpoint(void)
{
#ifdef DEBUG
	/* Detect Bochs with special I/O debug support */
	outw(0x8A00, 0x8A00);
	if (inw(0x8A00) == 0x8A00) {
		/* for simulator */
		outw(0x8A00, 0x8AE0);
	} else
#endif
	{
#ifdef USE_GDB_STUB
		__asm__("int $3");
#else
		hprintf("BREAKPOINT\n");
		for (;;) ;	/* hang */
#endif
	}
}
