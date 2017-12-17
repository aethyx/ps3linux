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
 * x86 specific support for gdbstub
 *
 */

#include <config.h>
#include <types.h>
#include <gdbstub.h>
#include <lib.h>
#include <mmu.h>
#include <io_chan.h>
#include <io.h>
#include <regs.h>
#include <idt.h>
#include <util.h>
#include <partition.h>

char gdb_stack[PGSIZE] __attribute__ ((aligned(PGSIZE)));

struct cpu_state *gdb_currdbg = (struct cpu_state *)(((char *)gdb_stack) +
						     sizeof (gdb_stack));

uval
gdb_signal_num(struct cpu_state *state, uval exception_type)
{
	(void)state;

	/* exception type identifies, trap or bad address */
	switch (exception_type) {
	case DB_VECTOR:
	case BP_VECTOR:
		return SIGTRAP;
	case UD_VECTOR:
		return SIGILL;
	case SO_VECTOR:
	case MF_VECTOR:
		return SIGFPE;
	case NP_VECTOR:
	case SS_VECTOR:
	case PF_VECTOR:
		return SIGSEGV;
	case MC_VECTOR:
		return SIGTERM;

	default:
		return SIGBUS;
	}
}

struct cpu_state *
gdb_resume(struct cpu_state *state, uval resume_addr, uval type)
{
	if (resume_addr != ~((uval)0)) {
		state->eip = resume_addr;
	}

	if (type == GDB_CONTINUE) {
		state->eflags &= ~EFLAGS_TF;
	} else {
		state->eflags |= EFLAGS_TF;
	}

	return state;
}

static void
write_to_packet_hex_le(uval x, uval width)
{
	char buf[sizeof (uval) * 2 + 1];
	uval i = 0;

	while (i < width) {
		buf[i++] = hex2char((x >> 4) & 15);
		buf[i++] = hex2char(x & 15);
		x >>= 8;
	}

	buf[i] = 0;
	write_to_packet(buf);
}

static uval __gdbstub
str_le2uval(const char *str, uval bytes)
{
	uval x = 0;
	uval i = 0;

	while (*str && i < (bytes * 2)) {
		if (!(i % 2)) {
			x |= char2hex(*str) << ((i + 1) * 4);
		} else {
			x |= char2hex(*str) << ((i - 1) * 4);
		}
		++i;
		++str;
	}

	return x;
}

void __gdbstub
gdb_write_to_packet_reg_array(struct cpu_state *state)
{
	uval *r = (uval *)state;

	while (r < (uval *)state + NR_CPU_STATE_REGS) {
		write_to_packet_hex_le(*r, sizeof (uval) * 2);
		r++;
	}
}

void __gdbstub
gdb_read_reg_array(struct cpu_state *state, char *buf)
{
	uval *r = (uval *)state;

	while (r < (uval *)state + NR_CPU_STATE_REGS) {
		*r = str_le2uval(buf, sizeof (uval));
		buf += sizeof (uval) * 2;
		r++;
	}
}

static inline uval
gdb_check_range(uval start, uval len)
{
	if (gdb_xlate_ea(start) == INVALID_VIRTUAL_ADDRESS) {
		return 0;
	}

	/* quick test if on same page */
	if ((start & (~0UL << LOG_PGSIZE)) ==
	    ((start + len) & (~0UL << LOG_PGSIZE))) {
		return 1;
	}

	/* we should never really loop here, but just in case */
	while (len > 0) {
		uval l = MIN(len, PGSIZE);

		if (gdb_xlate_ea(start + l) == INVALID_VIRTUAL_ADDRESS) {
			return 0;
		}
		len -= l;
	}
	return 1;
}

uval __gdbstub
gdb_write_to_packet_mem(struct cpu_state *state, const char *mem, uval len)
{
	(void)state;
	uval i;
	uval8 val;

	if (!gdb_check_range((uval)&mem[0], len)) {
		return 0;
	}

	for (i = 0; i < len; i++) {
		val = *(const uval8 *)&mem[i];
		write_to_packet_hex(val, 2);
	}
	return len;
}

uval __gdbstub
gdb_write_mem(struct cpu_state *state, char *mem, uval len, const char *buf)
{
	(void)state;
	uval j;
	uval8 val;

	if (!gdb_check_range((uval)&mem[0], len)) {
		return 0;
	}

	for (j = 0; j < len; j++) {
		val = str2uval(&buf[j * 2], 1);
		*(uval8 *)&mem[j] = val;
	}

	return len;
}

void __gdbstub
gdb_print_state(struct cpu_state *state)
{
	hprintf("EIP: 0x%08lx EFLAGS: 0x%08lx\n", state->eip, state->eflags);
	hprintf("EAX: 0x%08lx EBX: 0x%08lx\n", state->eax, state->ebx);
	hprintf("ECX: 0x%08lx EDX: 0x%08lx\n", state->ecx, state->edx);
	hprintf("ESP: 0x%08lx EBP: 0x%08lx\n", state->esp, state->ebp);
	hprintf("ESI: 0x%08lx EDI: 0x%08lx\n", state->esi, state->edi);

	hprintf("CS: 0x%08lx SS: 0x%08lx\n", state->cs, state->ss);
	hprintf("DS: 0x%08lx ES: 0x%08lx\n", state->ds, state->es);
	hprintf("FS: 0x%08lx GS: 0x%08lx\n", state->fs, state->gs);
}
