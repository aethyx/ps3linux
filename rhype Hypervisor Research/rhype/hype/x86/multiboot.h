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
 * Multiboot definitions.
 *
 */
#ifndef __HYPE_X86_MULTIBOOT_H__
#define __HYPE_X86_MULTIBOOT_H__

#define MBFLAGS_MEM_SUMMARY  (1 << 0)
#define MBFLAGS_BOOTDEV      (1 << 1)
#define MBFLAGS_CMDLINE      (1 << 2)
#define MBFLAGS_MODS         (1 << 3)
#define MBFLAGS_AOUTADDR     (1 << 4)
#define MBFLAGS_ELFADDR      (1 << 5)
#define MBFLAGS_MMAP         (1 << 6)
#define MBFLAGS_DRIVES       (1 << 7)

struct multiboot_info {
	uval32 flags;
	uval32 mem_lower;
	uval32 mem_upper;
	uval32 boot_device;
	uval32 cmdline;
	uval32 mods_count;
	uval32 mods_addr;
	union {
		struct {
			uval32 tabsize;
			uval32 strsize;
			uval32 addr;
			uval32 res0;
		} aout;
		struct {
			uval32 num;
			uval32 size;
			uval32 addr;
			uval32 shndx;
		} elf;
	} addr;
	uval32 mmap_length;
	uval32 mmap_addr;
	uval32 drives_length;
	uval32 drives_addr;
	uval32 config_table;
	uval32 boot_loader_name;
	uval32 apm_table;
	uval32 vbe_control_info;
	uval32 vbe_mode_info;
	uval32 vbe_mode;
	uval32 vbe_interface_seg;
	uval32 vbe_interface_off;
	uval32 vbe_interface_len;
};

struct mmap_info {
	uval size;
	uval32 base_addr_low;
	uval32 base_addr_hi;
	uval32 length_low;
	uval32 length_hi;
	uval type;
};

struct mod_info {
	uval32 mod_start;
	uval32 mod_end;
	uval32 string;
	uval32 res0;
};

#endif /* __HYPE_X86_MULTIBOOT_H__ */
