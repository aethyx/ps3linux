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
	DECLARE(GDB_MSR, offsetof(struct cpu_state, msr));
	DECLARE(GDB_PC, offsetof(struct cpu_state, pc));
	DECLARE(GDB_CR, offsetof(struct cpu_state, cr));
	DECLARE(GDB_XER, offsetof(struct cpu_state, xer));
	DECLARE(GDB_CTR, offsetof(struct cpu_state, ctr));
	DECLARE(GDB_LR, offsetof(struct cpu_state, lr));
	DECLARE(GDB_DAR, offsetof(struct cpu_state, dar));
	DECLARE(GDB_DSISR, offsetof(struct cpu_state, dsisr));
	DECLARE(GDB_GPR0, offsetof(struct cpu_state, gpr[0]));
	DECLARE(GDB_HSRR0, offsetof(struct cpu_state, hsrr0));
	DECLARE(GDB_HSRR1, offsetof(struct cpu_state, hsrr1));
	DECLARE(GDB_HDEC, offsetof(struct cpu_state, hdec));
	DECLARE(GDB_CPU_STATE_SIZE, sizeof (struct cpu_state));
#endif

	return 0;
}
