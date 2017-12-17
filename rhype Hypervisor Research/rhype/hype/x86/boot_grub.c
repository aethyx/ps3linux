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
 */

#include <config.h>
#include <boot.h>
#include <lib.h>
#include <pmm.h>
#include <vm.h>
#include <thinwire.h>

static struct memblock mem;

#if 0
static struct memblock *
memblock_get(uval i)
{
	if (i > 0) {
		return NULL;
	}
	return &mem;
}
#endif

static uval
memblock_init(uval m)
{
	mem.size = m;
	mem.addr = 0;
	return 1;
}

struct boot_grub_s {
	struct multiboot_info *bg_mb;
};

static struct boot_grub_s bg;

void *
boot_init(struct multiboot_info *mb)
{
	bg.bg_mb = mb;
	return &bg;
}

struct io_chan *
boot_console(void *b __attribute__ ((unused)))
{
#ifdef USE_VGA_CONSOLE
	return vga_init(0xb8000);
#else
	serial_init_fn = uartNS16750_init;
	return uartNS16750_init(0x3F8, 0, BAUDRATE);
#endif
}

static uval
find_mem_size(struct multiboot_info *mb)
{
	assert(mb->flags & MBFLAGS_MEM_SUMMARY,
	       "Missing bootloader memory summary\n");
	return mb->mem_upper;
}

uval
boot_memory(void *bp, uval img_start __attribute__ ((unused)),
	    uval img_end __attribute__ ((unused)))
{
	struct boot_grub_s *b = (struct boot_grub_s *)bp;
	struct multiboot_info *mb = b->bg_mb;
	uval kmemsz;

	kmemsz = find_mem_size(mb);

#ifdef DEBUG
	hprintf("Memory: %ld MB\n"
		"Chunk size: %ld MB\n"
		"Available partitions: %ld\n",
		kmemsz >> 10,
		CHUNK_SIZE >> 20,
		((kmemsz + (HV_LINK_BASE >> 10)) / (CHUNK_SIZE >> 10)) - 1);
#endif
	memblock_init(kmemsz * 1024);

	return kmemsz;
}

uval
boot_runtime_init(void *bp, uval eomem)
{
	(void)bp;
	return eomem;
}

struct io_chan *
boot_runtime_console(void *bofp __attribute__ ((unused)))
{
#ifdef USE_VGA_CONSOLE
	/* we cannot continue with VGA after this point */
	serial_init_fn = uartNS16750_init;
	return uartNS16750_init(0x3F8, 0, BAUDRATE);
#else
	/* return what we are currenlty using */
	return hout_get();
#endif
}

void
boot_shutdown(void)
{
	assert(0, "dunno how.\n");
}

void
boot_reboot(void)
{
	boot_shutdown();
}
