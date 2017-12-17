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
#include <lib.h>
#include <boot.h>
#include <asm.h>
#include <hv_regs.h>
#include <of_devtree.h>

struct boot_of {
	uval bo_r3;
	uval bo_r4;
	uval bo_r5;
	uval bo_r6;
	uval bo_r7;
	uval bo_hrmor;
	uval bo_msr;
	uval bo_memblks;
	struct memblock *bo_mem;
};

static struct boot_of bof;

extern void cpu_boot_init(uval r3, uval r4, uval r5, uval r6, uval r7,
			  uval boot_msr);

void *
boot_init(uval r3, uval r4, uval r5, uval r6, uval r7, uval boot_msr)
{
	cpu_boot_init(r3, r4, r5, r6, r7, boot_msr);

	/* save all parameters */
	bof.bo_r3 = r3;
	bof.bo_r4 = r4;
	bof.bo_r5 = r5;
	bof.bo_r6 = r6;
	bof.bo_r7 = r7;
	bof.bo_msr = boot_msr;

	bof.bo_hrmor = get_hrmor();
	return &bof;
}

struct io_chan *
boot_console(void *b __attribute__ ((unused)))
{
	return ofcon_init();
}

static struct memblock mem = {.addr = 0,.size = 512 << 20 };
uval
boot_memory(void *bp, uval img_start, uval img_end)
{
	(void)bp;
	(void)img_start;
	(void)img_end;
	return mem.size;
}

struct memblock *
boot_memblock(uval i)
{
	if (i > 0) {
		return NULL;
	}
	return &mem;
}

uval
boot_runtime_init(void *bp, uval eomem)
{
	(void)bp;
	return eomem;
}

static struct io_chan *console = NULL;

struct io_chan *
boot_runtime_console(void *bofp)
{
	(void)bofp;
	/* WHAT is this -JX */
	*(volatile uval *)16 = BOOT_CONSOLE_DEV_ADDR;
	if (console == NULL) {
		const char msg[] = "Metal Boot Environment\n";

#ifdef BOOT_CONSOLE_zilog
		console = zilog_init((uval)BOOT_CONSOLE_DEV_ADDR, 0, 0);
#else
#error Configure --with-boot-console-dev
#endif
		console->ic_write(console, msg, sizeof (msg));
	}
	return console;
}

sval
boot_devtree(uval of_mem, uval sz)
{
	return (sval)ofd_create((void *)of_mem, sz);
}

uval
boot_cpus(uval ofd)
{
	cpu_core_init(ofd);
}

extern uval _controller_start[0] __attribute__ ((weak));
extern uval _controller_end[0] __attribute__ ((weak));

sval
boot_controller(uval *start, uval *end)
{
#ifdef DEBUG
	hprintf("boot args: %lx %lx %lx %lx %lx\n",
		bof.bo_r3, bof.bo_r4, bof.bo_r5, bof.bo_r6, bof.bo_r7);
#endif
	*start = (uval)_controller_start;
	*end = (uval)_controller_end;
	return *end - *start;
}

void
boot_fini(void *bof)
{
	(void)bof;
	return;
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
