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
#include <gdbstub.h>
#include <io.h>
#include <bitops.h>
#include <lib.h>
#include <asm.h>

#define LOG_MAXPHYS (42 - 1)

uval __gdbstub
gdb_write_to_packet_mem_hv(struct cpu_state *state, const char *mem, uval len)
{
	(void)state;
	if ((uval)mem & (1ULL << LOG_MAXPHYS)) {
		uval i;
		uval bits;
		uval sz;
		uval64 val;

		bits = (len | (uval)mem);
		sz = 1UL << (ffs(bits) - 1);
		if (sz > sizeof (uval)) {
			sz = sizeof (uval);
		}

		for (i = 0; i < len; i += sz) {
			val = io_in_sz(&mem[i], sz);
			write_to_packet_hex(val, sz * 2);
		}
		return len;
	}
	return 0;
}

uval __gdbstub
gdb_write_mem_hv(struct cpu_state *state, char *mem, uval len, const char *buf)
{
	(void)state;
	uval j;

	if ((uval)mem & (1ULL << LOG_MAXPHYS)) {
		uval bits;
		uval sz;
		uval64 val;

		bits = (len | (uval)mem);
		sz = 1UL << (ffs(bits) - 1);
		if (sz > sizeof (uval)) {
			sz = sizeof (uval);
		}

		for (j = 0; j < len; j += sz) {
			val = str2uval(&buf[j], sz);
			io_out_sz(&mem[j], val, sz);

			if ((((uval)(&mem[j])) % CACHE_LINE_SIZE) == 0) {
				hw_dcache_flush((uval)&mem[j - sz]);
			}
		}

		/* one more for good luck */
		hw_dcache_flush((uval)&mem[j - sz]);

		return len;
	}
	return 0;
}
