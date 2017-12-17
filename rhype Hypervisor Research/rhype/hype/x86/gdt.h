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
 *
 * Global-descriptor-table support
 *
 */
#ifndef __HYPE_X86_GDT_H__
#define __HYPE_X86_GDT_H__

#include <config.h>
#include <types.h>
#include <regs.h>

enum {
	GDT_ENTRY_COUNT = 32,	/* number of entries in partition gdt */
	GDT_ENTRY_MIN = 4	/* first entry partition can modify */
};

struct gdt_entry_info {
	uval is_partial;
	uval32 last_toggle_eip;
	uval toggle_count;
};

#define SEG_FLAGS_DPL_MASK 0x0060
#define SEG_FLAGS_DPL_SHIFT 5
#define SEG_FLAGS_G_BIT 0x8000
#define SEG_FLAGS_P_BIT 0x0080
#define SEG_FLAGS_NONSYS_BIT 0x0010
#define SEG_FLAGS_CODE_BIT 0x0008
#define SEG_FLAGS_E_BIT 0x0004

static inline void
pack_descr(struct descriptor *descr_p, uval32 flags, uval32 base, uval32 size)
{
	descr_p->word0 = ((base & 0x0000ffff) << 16) |
		((size - 1) & 0x0000ffff);
	descr_p->word1 = (base & 0xff000000) | ((base & 0x00ff0000) >> 16) |
		((size - 1) & 0x000f0000) | ((flags & 0xf0ff) << 8);
}

static inline void
unpack_descr_words(uval32 word0, uval32 word1,
		   uval32 *flags_p, uval32 *base_p, uval32 *size_p)
{
	if (flags_p != NULL) {
		(*flags_p) = (word1 >> 8) & 0xf0ff;
	}
	if (base_p != NULL) {
		(*base_p) = (word1 & 0xff000000) |
			((word1 << 16) & 0x00ff0000) |
			((word0 >> 16) & 0x0000ffff);
	}
	if (size_p != NULL) {
		(*size_p) = ((word1 & 0x000f0000) | (word0 & 0x0000ffff)) + 1;
	}
}

static inline void
unpack_descr(struct descriptor *descr_p,
	     uval32 *flags_p, uval32 *base_p, uval32 *size_p)
{
	unpack_descr_words(descr_p->word0, descr_p->word1,
			   flags_p, base_p, size_p);
}

struct cpu_thread;		/* forward declaration */
void gdt_init(struct cpu_thread *thread);
uval handle_partial_seg_fault(struct cpu_thread *thread);

#endif /* __HYPE_X86_GDT_H__ */
