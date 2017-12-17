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

#ifndef _GDBSTUB_H
#define _GDBSTUB_H
/* Append this to the declaration of any function needed to be used by
 * the gdb stub.  Any functions marked with this will not be allowed
 * to have a breakpoint set in them. */
#define __gdbstub __attribute__((section("gdbstub")))

extern void __gdbstub gdb_stub_init(void);
extern struct cpu_state *__gdbstub enter_gdb(struct cpu_state *state,
					     uval exception_type);

extern void __gdbstub write_to_packet(const char *str);
extern void __gdbstub write_to_packet_hex(uval x, uval min_width);

extern void __gdbstub gdb_write_to_packet_reg_array(struct cpu_state *state);
extern void __gdbstub gdb_read_reg_array(struct cpu_state *state, char *buf);
extern uval __gdbstub gdb_write_to_packet_mem(struct cpu_state *state,
					      const char *addr, uval len);
extern uval __gdbstub gdb_write_to_packet_mem_hv(struct cpu_state *state,
						 const char *addr, uval len)
	__attribute__ ((weak));
extern uval __gdbstub gdb_write_mem(struct cpu_state *state, char *addr,
				    uval len, const char *buf);
extern uval __gdbstub gdb_write_mem_hv(struct cpu_state *state, char *addr,
				       uval len, const char *buf)
	__attribute__ ((weak));
extern uval __gdbstub gdb_xlate_ea(uval eaddr);

extern sval __gdbstub gdb_io_write(const char *buf, uval len);
extern sval __gdbstub gdb_io_read(char *buf, uval len);

/* Signal number to return to gdb.  Also perform any post-trap fixup
 * of program counter. */
extern uval __gdbstub gdb_signal_num(struct cpu_state *state,
				     uval exception_type);
extern void __gdbstub gdb_enter_notify(uval exception_type);

extern void __gdbstub gdb_print_state(struct cpu_state *state);

#define GDB_CONTINUE	 0
#define GDB_STEP	 1
struct cpu_state *__gdbstub gdb_resume(struct cpu_state *state,
				       uval resume_addr, uval type);

#define SIGILL		 4
#define SIGTRAP		 5
#define SIGBUS		 7
#define SIGFPE		 8	/* floating point */
#define SIGSEGV		11
#define SIGALRM		14
#define SIGTERM		15

#endif /* _GDBSTUB_H */
