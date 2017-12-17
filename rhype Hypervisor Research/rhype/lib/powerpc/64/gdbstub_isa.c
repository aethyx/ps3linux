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
 * ppc64 specific support for gdbstub
 *
 */
#include <config.h>
#include <types.h>
#include <gdbstub.h>
#include <lib.h>
#include <mmu.h>
#include <io_chan.h>
#include <io.h>
#include <bitops.h>

char gdb_stack[PGSIZE] __attribute__ ((aligned(PGSIZE)));

struct cpu_state *gdb_currdbg =
	(struct cpu_state *)(((char *)gdb_stack) + sizeof (gdb_stack));

extern uval32 trap_instruction[];

static inline uval __gdbstub
gdb_ppc_0x700(struct cpu_state *state)
{
	uval instr;

	switch (state->msr & MSR_TRAP_BITS) {
	case MSR_TRAP_FE:
		return SIGFPE;
	case MSR_TRAP_IOP:
	case MSR_TRAP_PRIV:
		return SIGILL;
	case MSR_TRAP:
		instr = *((uval32 *)state->pc);

		if (instr == *trap_instruction) {
			state->pc += sizeof (uval32);
		}
		return SIGTRAP;
	}
	return SIGBUS;
}

uval __gdbstub
gdb_signal_num(struct cpu_state *state, uval exception_type)
{
	state->curmsr = mfmsr();
	/* exception type identifies, trap or bad address */
	switch (exception_type) {
	case 0x200: /* Machine Check */
		return SIGTERM;
	case 0x300: /* DSI */
	case 0x380: /* Data SLB */
	case 0x400: /* ISI */
	case 0x480: /* Instruction SLB */
		return SIGSEGV;
	case 0x600: /* Alignment SLB */
		return SIGBUS;
	case 0x700: /* Program */
		return gdb_ppc_0x700(state);
	case 0x800: /* Float */
		return SIGFPE;
	case 0x900: /* Decrementer */
		return SIGALRM; /* is this right? */
	case 0xd00: /* TRAP */
		return SIGTRAP;
	case 0xe00: /* FP */
		return SIGFPE;
	}
	return SIGBUS;
}


struct cpu_state * __gdbstub
gdb_resume(struct cpu_state *state, uval resume_addr, uval type)
{
	if (resume_addr != ~((uval)0)) {
		state->pc = resume_addr;
	}

	if (type == GDB_CONTINUE) {
		state->msr &= ~MSR_SE;
	} else {
		state->msr |= MSR_SE;
	}
	return state;
}


void __gdbstub
gdb_write_to_packet_reg_array(struct cpu_state *state)
{
	uval i = 0;

	for (i = 0; i < 32; ++i) {
		write_to_packet_hex(state->gpr[i], sizeof (state->gpr[0]) * 2);
	}
	/* Avoid floating point for now */
	for (i = 0; i < 32; ++i) {
		write_to_packet_hex(0, sizeof (uval64) * 2);
	}
	write_to_packet_hex(state->pc, sizeof (state->pc) * 2);
	write_to_packet_hex(state->ps, sizeof (state->ps) * 2);
	write_to_packet_hex(state->cr, sizeof (state->cr) * 2);
	write_to_packet_hex(state->lr, sizeof (state->lr) * 2);
	write_to_packet_hex(state->ctr, sizeof (state->ctr) * 2);
	write_to_packet_hex(state->xer, sizeof (state->xer) * 2);
	write_to_packet_hex(0, sizeof(uval32) * 2); /* fpscr */
}


void __gdbstub
gdb_read_reg_array(struct cpu_state *state, char *buf)
{
	uval i;

	for (i = 0; i < 32; ++i) {
		state->gpr[i] = str2uval(buf, sizeof (uval));
		buf += sizeof (state->gpr[0]) * 2;
	}
	/* Avoid floating point for now */
	for (i = 0; i < 32; ++i) {
		buf += sizeof (uval64) * 2;
	}

	state->pc = str2uval(buf, sizeof (state->pc));
	buf += sizeof (state->pc) * 2;
	state->ps = str2uval(buf, sizeof (state->ps));
	buf += sizeof (state->ps) * 2;
	state->cr = str2uval(buf, sizeof (state->cr));
	buf += sizeof (state->cr) * 2;
	state->lr = str2uval(buf, sizeof (state->lr));
	buf += sizeof (state->lr) * 2;
	state->ctr = str2uval(buf, sizeof (state->ctr));
	buf += sizeof (state->ctr) * 2;
	state->xer = str2uval(buf, sizeof (state->xer));
	buf += sizeof (state->xer) * 2;
}


uval __gdbstub
gdb_write_to_packet_mem(struct cpu_state *state, const char *mem, uval len)
{
	(void)state;
	uval i;
	uval bits;
	uval sz;

	if (gdb_write_to_packet_mem_hv) {
		if (gdb_write_to_packet_mem_hv(state, mem, len) == len) {
			return len;
		}
	}

	bits = (len | (uval)mem);
	sz = 1UL << (ffs(bits) - 1);
	if (sz > sizeof (uval)) {
		sz = sizeof (uval);
	}

	for (i = 0; i < len; i += sz) {
		uval64 val;

		switch (sz) {
		default:  /* cannot happen */
			val = 0;
			break;
		case sizeof (uval8):
			val = *(const uval8 *)&mem[i];
			break;
		case sizeof (uval16):
			val = *(const uval16 *)&mem[i];
			break;
		case sizeof (uval32):
			val = *(const uval32 *)&mem[i];
			break;
		case sizeof (uval64):
			val = *(const uval64 *)&mem[i];
			break;
		}
		write_to_packet_hex(val, sz * 2);
	}

	return len;
}

uval __gdbstub
gdb_write_mem(struct cpu_state *state, char *mem, uval len, const char *buf)
{
	(void)state;
	uval j;
	uval bits;
	uval sz;
	uval64 val;

	if (gdb_write_mem_hv) {
		if (gdb_write_mem_hv(state, mem, len, buf) == len) {
			return len;
		}
	}

	/*
	 * Try to find the best native load/store size.  This is so if
	 * we are looking at a piece of memory that is sensitive to
	 * its method of access (like an MMIO register, we will
	 * hopefully get the size right. This increases our chances
	 * but is no guarantee.
	 */

	bits = (len | (uval)mem);
	sz = 1UL << (ffs(bits) - 1);
	if (sz > sizeof (uval)) {
		sz = sizeof (uval);
	}

	for (j = 0; j < len; j += sz) {
		val = str2uval(&buf[j], sz);

		switch (sz) {
		case sizeof (uval8):
			*(uval8 *)&mem[j] = val;
			break;
		case sizeof (uval16):
			*(uval16 *)&mem[j] = val;
			break;
		case sizeof (uval32):
			*(uval32 *)&mem[j] = val;
			break;
		case sizeof (uval64):
			*(uval64 *)&mem[j] = val;
			break;
		}
		if ((((uval)&mem[j]) % CACHE_LINE_SIZE) == 0) {
			hw_dcache_flush((uval)&mem[j - sz]);
		}
	}

	/* one more for good luck */
	hw_dcache_flush((uval)&mem[j - sz]);

	return len;
}


void __gdbstub
gdb_print_state(struct cpu_state *state)
{
	int i = 0;
	hprintf("PC: 0x%016lx MSR: 0x%016lx\n", state->pc, state->msr);
	hprintf("LR: 0x%016lx CTR: 0x%016lx\n", state->lr, state->ctr);
	hprintf("DAR: 0x%016lx DSISR: 0x%016lx\n", state->dar, state->dsisr);
	hprintf("CR: 0x%08x XER: 0x%08x\n", state->cr, state->xer);
	for (; i < 32; i+=4) {
		hprintf("%02d: 0x%016lx 0x%016lx 0x%016lx 0x%016lx\n",
			i, state->gpr[i], state->gpr[i+1],
			state->gpr[i+2], state->gpr[i+3]);
	}
}
