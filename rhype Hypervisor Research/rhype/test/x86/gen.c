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
#include <types.h>
#include <asm.h>
#include <util.h>
#include <gdbstub.h>

int
main(void)
{
	DECLARE(GDB_ZERO, 0);
#ifdef USE_GDB_STUB
	DECLARE(GDB_EAX, offsetof(struct cpu_state, eax));
	DECLARE(GDB_ECX, offsetof(struct cpu_state, ecx));
	DECLARE(GDB_EDX, offsetof(struct cpu_state, edx));
	DECLARE(GDB_EBX, offsetof(struct cpu_state, ebx));
	DECLARE(GDB_ESP, offsetof(struct cpu_state, esp));
	DECLARE(GDB_EBP, offsetof(struct cpu_state, ebp));
	DECLARE(GDB_ESI, offsetof(struct cpu_state, esi));
	DECLARE(GDB_EDI, offsetof(struct cpu_state, edi));
	DECLARE(GDB_EIP, offsetof(struct cpu_state, eip));
	DECLARE(GDB_EFLAGS, offsetof(struct cpu_state, eflags));
	DECLARE(GDB_CS, offsetof(struct cpu_state, cs));
	DECLARE(GDB_SS, offsetof(struct cpu_state, ss));
	DECLARE(GDB_DS, offsetof(struct cpu_state, ds));
	DECLARE(GDB_ES, offsetof(struct cpu_state, es));
	DECLARE(GDB_FS, offsetof(struct cpu_state, fs));
	DECLARE(GDB_GS, offsetof(struct cpu_state, gs));
#endif /* USE_GDB_STUB */

	return 0;
}
