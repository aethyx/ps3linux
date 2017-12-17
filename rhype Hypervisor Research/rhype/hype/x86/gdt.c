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
 * GDT Handling
 *
 */

#include <config.h>
#include <hype.h>
#include <pmm.h>
#include <vm.h>
#include <gdt.h>
#include <h_proto.h>

#undef GDT_DEBUG

void
gdt_init(struct cpu_thread *thread)
{
	memset(thread->gdt, 0, sizeof (thread->gdt));
	pack_descr(&thread->gdt[__HV_CS >> 3], 0xc09a, 0, 0x100000);
	pack_descr(&thread->gdt[__HV_DS >> 3], 0xc092, 0, 0x100000);
	pack_descr(&thread->gdt[__TSS >> 3],
		   0x89, (uval32)&thread->tss, sizeof (thread->tss));
	pack_descr(&thread->gdt[__GUEST_CS >> 3], 0xc0da, 0, 0xfc000);
	pack_descr(&thread->gdt[__GUEST_DS >> 3], 0xc0d2, 0, 0xfc000);

	thread->gdtr.base = (uval32)thread->gdt;
	thread->gdtr.size = sizeof (thread->gdt) - 1;

	memset(thread->gdt_info, 0, sizeof (thread->gdt_info));
}

sval
h_gdt_entry(struct cpu_thread *thread, uval index, uval word0, uval word1)
{
	uval32 flags,
	       base,
	       size,
	       max_size;

#ifdef GDT_DEBUG
	/* temporary: print toggle count */
	if ((index >= GDT_ENTRY_MIN) && (index < GDT_ENTRY_COUNT) &&
	    (thread->gdt_info[index].is_partial)) {
		hprintf("\t\tpartial segment %ld toggled %ld times.\n",
			index, thread->gdt_info[index].toggle_count);
	}
#endif

#ifdef GDT_DEBUG
	hprintf("h_gdt_entry: lpid 0x%x: index %ld, word0 0x%lx, word1 0x%lx\n", thread->cpu->os->po_lpid, index, word0, word1);
#endif

	if ((index < GDT_ENTRY_MIN) || (index >= GDT_ENTRY_COUNT)) {
		return H_Parameter;
	}

	if ((word0 == 0) && (word1 == 0)) {
		thread->gdt[index].word0 = 0;
		thread->gdt[index].word1 = 0;
		thread->gdt_info[index].is_partial = 0;
		return H_Success;
	}

	unpack_descr_words(word0, word1, &flags, &base, &size);

	/*
	 * Recognize "partial" segments.  A partial segment is a user-mode
	 * data segment that covers the full 4-gig address space but has a
	 * non-zero base.  We cannot fully map such a segment without making
	 * hypervisor space accessible to the client.  We can only map part of
	 * it at a time, hence the term "partial segment".  Once we exclude
	 * hypervisor space, we can map either valid positive offsets from the
	 * base address or valid negative offsets, but not both at the same
	 * time.
	 *
	 * Our approach (suggested by Marc Auslander) is to initially map
	 * valid positive offsets.  If the client uses a negative offset, the
	 * processor will raise a general protection fault, and our fault
	 * handler will change the gdt entry to map the negative offsets.  At
	 * the next fault we change it back, and so on.  In this way we
	 * support all valid accesses to the segment, albeit at the cost of
	 * a general protection fault every time the client changes the
	 * polarity of its accesses.
	 */
	if ((base != 0) &&
	    (base <= HV_VBASE) &&
	    (size == 0x100000) &&
	    ((flags & (SEG_FLAGS_P_BIT |
		       SEG_FLAGS_G_BIT |
		       SEG_FLAGS_NONSYS_BIT |
		       SEG_FLAGS_CODE_BIT |
		       SEG_FLAGS_E_BIT |
		       SEG_FLAGS_DPL_MASK)) == (SEG_FLAGS_P_BIT |
						SEG_FLAGS_G_BIT |
						SEG_FLAGS_NONSYS_BIT |
						(3 << SEG_FLAGS_DPL_SHIFT)))) {
		size = (HV_VBASE - base) >> 12;
		pack_descr(&thread->gdt[index], flags, base, size);
		thread->gdt_info[index].is_partial = 1;
		thread->gdt_info[index].last_toggle_eip = 0;
		thread->gdt_info[index].toggle_count = 0;

		return H_Success;
	}

	/* If the segment is "present", enforce some restrictions. */
	if ((flags & SEG_FLAGS_P_BIT) != 0) {

		/* We can't handle "expand down" data segments. */
		if (((flags & (SEG_FLAGS_NONSYS_BIT |
			       SEG_FLAGS_CODE_BIT |
			       SEG_FLAGS_E_BIT)) == (SEG_FLAGS_NONSYS_BIT |
						     SEG_FLAGS_E_BIT))) {
			return H_Parameter;
		}

		/* Force segment privilege level to at least 2. */
		if ((flags & SEG_FLAGS_DPL_MASK) < (2 << SEG_FLAGS_DPL_SHIFT)) {
			flags = (flags & ~SEG_FLAGS_DPL_MASK) |
				(2 << SEG_FLAGS_DPL_SHIFT);
		}

		/* Restrict segment range to exclude HV space. */
		max_size = (base > HV_VBASE) ? 0 : HV_VBASE - base;
		if ((flags & SEG_FLAGS_G_BIT) != 0)
			max_size >>= 12;
		if (size > max_size)
			size = max_size;
	}

	pack_descr(&thread->gdt[index], flags, base, size);
	thread->gdt_info[index].is_partial = 0;

	return H_Success;
}

/*
 * Called from the general-protection-fault handler.  See the discussion of
 * "partial segments" in h_gdt_entry().  This routine tries to determine
 * whether the current fault is a fault on a partial segment.  If so, it
 * "handles" the fault by toggling the polarity of supported segment offsets
 * and returns 1.  Otherwise it returns 0.
 *
 * The hardware gives us no help in distinguishing partial-segment faults from
 * other general protection faults, leaving us no recourse but to examine the
 * faulting instruction.  For now, we get by with a very crude instruction
 * parse.  In practice, partial-segment faults invariably result from
 * references through the %fs or %gs segment registers, so we just look for
 * an %fs or %gs segment override among the instruction prefix bytes.  If we
 * find one, and if the corresponding segment register refers to a partial
 * segment, we consider this a partial-segment fault.  We toggle the segment
 * polarity and restart the client.
 *
 * We also have to deal with the complementary problem, deciding that a fault
 * is a partial-segment fault when in fact it's something else, perhaps a
 * truly invalid access to hypervisor space.  We record the address of the
 * faulting instruction when we toggle a partial segment, and if the next fault
 * occurs at the same address we don't toggle the segment again and instead
 * reflect the fault.  It's possible for a single instruction to make both
 * positive and negative references at different times, but it's not happening
 * in practice, and until it does, this simple loop detection is good enough.
 */
uval
handle_partial_seg_fault(struct cpu_thread *thread)
{
	uval i,
	     pc,
	     seg,
	     idx;
	uval8 inst_byte;
	uval32 flags,
	       base,
	       size;

	/*
	 * Read as many as 4 bytes from the faulting instruction, looking for
	 * %fs and %gs segment override prefixes and skipping other prefixes.
	 */
	pc = thread->tss.eip;
	seg = 0;
	for (i = 0; i < 4; i++) {	/* Look at 4 bytes at most */
		/*
		 * We're here because of a general protection fault, so we
		 * know the processor has successfully read the faulting
		 * instruction and that it's safe to read the instruction
		 * ourselves.  On a multiprocessor, we may need to lock the
		 * page table and then check that the mapping is still valid.
		 */
		inst_byte = *((uval8 *)pc);

		if ((inst_byte == 0x66) ||	/* Operand size override prefix */
		    (inst_byte == 0x67) ||	/* Address size override prefix */
		    (inst_byte == 0xF0) ||	/* LOCK prefix */
		    (inst_byte == 0xF2) ||	/* REPNE/REPNZ prefix */
		    (inst_byte == 0xF3)) {	/* REP/REPE/REPZ prefix */
			pc++;
			continue;
		}

		if (inst_byte == 0x64) {	/* FS segment override prefix */
			seg = thread->tss.srs.regs.fs;
			break;
		}
		if (inst_byte == 0x65) {	/* GS segment override prefix */
			seg = thread->tss.srs.regs.gs;
			break;
		}

		/*
		 * We've reached either the opcode or a non-FS,non-GS segment
		 * override prefix.
		 */
		break;
	}

	idx = seg >> 3;
	if ((idx >= GDT_ENTRY_MIN) &&	/* client-modifiable segment */
	    (idx < GDT_ENTRY_COUNT) && ((seg & 0x4) == 0) &&	/* segment in gdt, not ldt */
	    (thread->gdt_info[idx].is_partial) &&
	    (thread->tss.eip != thread->gdt_info[idx].last_toggle_eip)) {
		unpack_descr(&thread->gdt[idx], &flags, &base, &size);
		if ((flags & SEG_FLAGS_E_BIT) == 0) {
			/* currently an "up" segment; change to "down" */
			flags |= SEG_FLAGS_E_BIT;
			size = 0x100000 - (base >> 12);
		} else {
			/* currently a "down" segment; change to "up" */
			flags &= ~SEG_FLAGS_E_BIT;
			size = (HV_VBASE - base) >> 12;
		}
		pack_descr(&thread->gdt[idx], flags, base, size);
		/* record the faulting eip to catch loops */
		thread->gdt_info[idx].last_toggle_eip = thread->tss.eip;
		thread->gdt_info[idx].toggle_count++;
		return 1;
	}

	return 0;
}
