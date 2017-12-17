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

/*
 * exceptions.S
 * low level exception handling code for test OSs
 */

.text
.global default_handler
default_handler:
	int	$0xF1		// iret
	jmp	.

.global async_handler
async_handler:
	pusha
	pushf
	call aipc_handler
	popf
	popa
	int	$0xF1		// iret
	jmp	.

tmp:
	.long 	0

.global pfault_handler
pfault_handler:
	pusha
	call do_pfault
	popa
	popl 	tmp		// fault address
	popl	tmp
	int	$0xF1		// iret
	jmp	.
