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
#include <asm.h>

/* One day we will but specific initialization code here */
	
C_TEXT_ENTRY(_start)
	/* load up the stack */
	LOADADDR(r1, _hype_stack_start)
	subi r1, r1, MIN_FRAME_SZ

	/* call the init function */
	LOADADDR(r21,hype_init)
	CALL_CFUNC(r21)

C_TEXT_END(_start)
