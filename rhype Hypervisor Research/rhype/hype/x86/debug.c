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
 * Various useful debug routines.
 *
 */
#include <config.h>
#include <lib.h>
#include <cpu_thread.h>
#include <mmu.h>
#include <pmm.h>
#include <vm.h>
#include <debug.h>
#include <h_proto.h>

#ifdef DEBUG
void
dump_stack(struct cpu_thread *thread, int low, int high)
{
	uval esp;
	uval end;
	uval addr = thread->tss.gprs.regs.esp;

	hprintf("Stack dump (lpid 0x%x, esp 0x%lx):\n",
		thread->cpu->os->po_lpid, addr);
	esp = addr + low * sizeof (uval);
	end = addr + high * sizeof (uval);
	for (; esp < end; esp += sizeof (uval)) {
		uval val;

		if (get_lpar(thread, &val, esp, sizeof (uval)) != NULL) {
			hprintf("V0x%08lx: P0x%08lx:\t0x%08lx",
				esp, v2p(thread, esp), val);
		} else
			hprintf("0x%08lx: invalid address", esp);
		if (esp == addr)
			hprintf("\t<--- current");
		hprintf("\n");
	}
}

void
dump_cpu_state(struct cpu_thread *thread)
{
	hprintf("CPU state for LPID 0x%x:\n", thread->cpu->os->po_lpid);
	hprintf("EAX: 0x%08lx      EBX: 0x%08lx\n",
		thread->tss.gprs.regs.eax, thread->tss.gprs.regs.ebx);
	hprintf("ECX: 0x%08lx      EDX: 0x%08lx\n",
		thread->tss.gprs.regs.ecx, thread->tss.gprs.regs.edx);
	hprintf("ESI: 0x%08lx      EDI: 0x%08lx\n",
		thread->tss.gprs.regs.esi, thread->tss.gprs.regs.edi);
	hprintf("ESP: 0x%08lx      EBP: 0x%08lx\n",
		thread->tss.gprs.regs.esp, thread->tss.gprs.regs.ebp);

	hprintf(" DS: 0x%08x       ES: 0x%08x\n",
		thread->tss.srs.regs.ds, thread->tss.srs.regs.es);

	hprintf("EFLAGS: 0x%lx\n", thread->tss.eflags);
	hprintf("CS : 0x%x: EIP: 0x%lx\n",
		thread->tss.srs.regs.cs, thread->tss.eip);
	hprintf("CR3: 0x%lx\n", thread->tss.cr3);
}

void
hexdump(unsigned char *data, int sz)
{
	unsigned char *d;
	int i;

	for (d = data; sz > 0; d += 16, sz -= 16) {
		int n = sz > 16 ? 16 : sz;

		hprintf("%08lx: ", (uval)d);
		for (i = 0; i < n; i++)
			hprintf("%02x%c", d[i], i == 7 ? '-' : ' ');
		for (; i < 16; i++)
			hprintf("  %c", i == 7 ? '-' : ' ');
		hprintf("   ");
		for (i = 0; i < n; i++)
			hprintf("%c", d[i] >= ' ' && d[i] <= '~' ? d[i] : '.');
		hprintf("\n");
	}
}

uval
probe_reg(struct cpu_thread *thread, uval idx)
{
	/* IMPLEMENT ME!!!!! */
	/* Return register contents , indicated by idx of "thread". */
	/* See ppc implementation. */
	(void)thread;
	(void)idx;

	return 0;
}

#endif /* DEBUG */
