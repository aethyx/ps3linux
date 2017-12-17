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
 * lboot.c
 *
 * Linux bootstrap loader for the x86 version of the Research
 * Hypervisor. Essentially sets up the environment Linux expects and
 * passes on the parameters. This could evolve into something as
 * complex as grub, or even be replaced by a grub port. Why not? A
 * grub that knows about vether and vscsi might actually be a very
 * useful thing.
 */
#include <lboot.h>
#include <partition.h>
#include <asm.h>
#include <util.h>
#include <lib.h>
#include <mmu.h>
#include <hcall.h>
#include <mailbox.h>

#define	LOADADDR	0x100000

/*
 * Linux initially maps 4MB and assumes everything it needs
 * is contained in that. The bootparams are stored in the
 * last page of that 4MB. The ramdisk image can be anywhere
 * because it is not access until all memory is mapped by
 * Linux.
 */
#define	MAX_KERNEL_SIZE	(1023 * PGSIZE)
#define	MIN_MEMORY_SIZE	(1024 * PGSIZE)

/* Linux doesn't like the ramdisk image to be higher than 896MB */
#define	MAX_LOW_MEMORY	(0x38000000)

/*
 * Linux bootstrap parameters
 */
#define	VIDEO_MODE	(*(uval8 *)(boot_params + 0x006))
#define	VIDEO_COLS	(*(uval8 *)(boot_params + 0x007))
#define	VIDEO_EGA_BX	(*(uval16 *)(boot_params + 0x00A))
#define	VIDEO_LINES	(*(uval8 *)(boot_params + 0x00E))
#define	VIDEO_ISVGA	(*(uval8 *)(boot_params + 0x00F))
#define	VIDEO_POINTS	(*(uval16 *)(boot_params + 0x010))
#define	LOADER_TYPE	(*(uval32 *)(boot_params + 0x210))
#define	INITRD_START	(*(uval32 *)(boot_params + 0x218))
#define	INITRD_SIZE	(*(uval32 *)(boot_params + 0x21c))
#define	NEW_CL_POINTER	(*(uval32 *)(boot_params + 0x228))
#define	NEW_VD_POINTER  (*(uval32 *)(boot_params + 0x22c))
#define	COMMAND_LINE	((char *)(boot_params + 0x800))
#define	VDEV_DATA       ((char *)(boot_params + 0x900))

unsigned long ksize;
unsigned long isize;
int noinitrd = 0;
unsigned long memsize = 0;
unsigned char *kernel;
unsigned char *initrd;
unsigned char *boot_params;
struct partition_info partinfo[2];

extern unsigned char kernel_start[], kernel_end[], kernel_size[];
extern unsigned char initrd_start[], initrd_end[], initrd_size[];

#ifdef DEBUG
static inline void
outw(uval16 addr, uval16 val)
{
	__asm__ __volatile__ ( "outw %%ax, %%dx" :: "d"(addr), "a"(val));
}

static inline void
breakpoint(void)
{
	outw(0x8A00, 0x8AE0);
}
#endif /* DEBUG */

static void
halt(void)
{
	hprintf("Halting ...\n");
	for (;;) hcall_yield(NULL, H_SELF_LPID);
}

static void
partition_info(void)
{
	struct partition_info *p = &partinfo[1];
	int i, plus = 0;

#ifdef notneeded
	hcall_lpar_info(NULL, H_GET_PINFO, (uval) &partinfo[1], 0, 0, 0);
#endif

	hprintf("Partition ID: 0x%x\n", p->lpid);
	hprintf("Memory: ");
	for (i = 0; i < MAX_MEM_RANGES; i++) {
		if (p->mem[i].size != INVALID_MEM_RANGE) {
			if (plus++)
				hprintf(" + ");
			hprintf("%ld MB", p->mem[i].size >> 20);
			memsize += p->mem[i].size;
#ifdef SPLIT_MEMORY
			break;
#endif
		}
	}
	hprintf(" = %ld MB\n", memsize >> 20);
}

/*
 * Inline assembly versions to speed up the simulator
 */
static inline void
move(void *dest, void *src, int count)
{
	int t0, t1, t2;

	__asm__ __volatile__(
		"cld\n"
		"rep; movsl\n"
		"testb $2,%b4\n"
		"je 1f\n"
		"movsw\n"
		"1: testb $1,%b4\n"
		"je 2f\n"
		"movsb\n"
		"2:"
		: "=&c" (t0), "=&D" (t1), "=&S" (t2)
		: "0" (count/4), "q" (count), "1" ((long)dest), "2" ((long)src)
		: "memory"
	);
}

static inline void
clear(void *dest, int count)
{
	int d0, d1;
	__asm__ __volatile__(
		"cld\n"
		"rep; stosb"
	        : "=&c" (d0), "=&D" (d1)
	        :"a" (0),"1" (dest),"0" (count)
	        :"memory");
}

static inline int
findstr(const char *s, const char *f)
{
	while (*s) {
		if (*s == *f) {
			int l = 0;
			while (*s == *f && *f != '\0') {
				l++;
				s++; f++;
			}
			if (*f == '\0')
				return l;
			s -= l; f -= l;
		}
		s++;
	}

	return 0;
}

static void
move_kernel(void)
{
	kernel = (unsigned char *) LOADADDR;
	move(kernel, kernel_start, ksize);
}

#ifdef RAMDISK
static void
move_initrd(void)
{
	unsigned long top;
	uval isize = (uval)initrd_size;

	top = memsize > MAX_LOW_MEMORY ? MAX_LOW_MEMORY : memsize;

	initrd = (unsigned char *) top - isize - PGSIZE;
	clear(initrd, isize);
	move(initrd, initrd_start, (uval)initrd_end - (uval)initrd_start);
}
#endif /* RAMDISK */

/*
 * Fill in the parameters that Linux expects. Passing the
 * partition info is our own invention.
 */
static void
linux_parameters(const struct os_args *my_os_args)
{
	uval i;

	boot_params = (unsigned char *) 0x1000; // was MAX_KERNEL_SIZE

	clear(boot_params, PGSIZE);

#ifdef DEBUG
	printf("Memory map:\n");
	printf("\t0x%x..0x%x\t(Kernel image)\n",
		(int) kernel, (int) kernel + (kernel_end - kernel_start));
	printf("\t0x%x..0x%x\t(Boot parameters)\n",
		(int) boot_params, (int) boot_params + PGSIZE);
#ifdef RAMDISK
	if (!noinitrd) {
		printf("\t0x%x..0x%x\t(Initial ramdisk image)\n", (int) initrd,
			(int) initrd + (initrd_end - initrd_start));
	}
#endif
#endif /* DEBUG */

	hprintf("Command line: %s\n", my_os_args->commandlineargs);

	LOADER_TYPE = 1;
	for (i = 0; i < strlen(my_os_args->commandlineargs)+1; i++)
		COMMAND_LINE[i] = my_os_args->commandlineargs[i];
	NEW_CL_POINTER = (unsigned) COMMAND_LINE;

	hprintf("vdev data: %s\n", my_os_args->vdevargs);
	for (i = 0; i < strlen(my_os_args->vdevargs)+1; i++)
		VDEV_DATA[i] = my_os_args->vdevargs[i];
	NEW_VD_POINTER = (unsigned) VDEV_DATA;

#ifdef RAMDISK
	if (!noinitrd) {
		INITRD_START = (unsigned) initrd;
		INITRD_SIZE = initrd_end - initrd_start;
	}
#endif /* RAMDISK */

	/*
	 * Video initialization under Linux is a royal pain!
	 * For now, I assume that we have some color VGA compatible
	 * card if we have I/O privileges.
	 */
	VIDEO_ISVGA = 1;
	VIDEO_MODE = 3;
	VIDEO_EGA_BX = 3;
	VIDEO_LINES = 25;
	VIDEO_COLS = 80;
	VIDEO_POINTS = 16;
}


static struct os_args my_os_args;

static struct os_args *
move_os_args(const struct os_args *orig_args)
{
	char *ptr = (char *)&my_os_args;
	uval i = 0;
	while (i < sizeof(struct os_args)) {
		ptr[i] = ((const char *)orig_args)[i];
		i++;
	}
	return &my_os_args;;
}

void
lboot(struct os_args *cmdline)
{
	hprintf("vHype/x86 Linux bootstrap loader\n");
	hprintf("Copyright (c) 2004 "
		"International Business Machines Corporation.\n");

	cmdline = move_os_args(cmdline);

	/* parse partition info */
	partition_info();

	/* some Linux options can be implemented here */
	noinitrd = findstr(cmdline->commandlineargs, "noinitrd");

	/* compute sizes and do basic sanity checks */
	if (memsize < MIN_MEMORY_SIZE) {
		hprintf("Not enough memory to boot Linux");
		halt();
	}

	ksize = (uval)kernel_size;
	if (ksize >= MAX_KERNEL_SIZE) {
		hprintf("Linux kernel does not fit within 4MB\n");
		halt();
	}

	/* move kernel to the proper location */
	move_kernel();

#ifdef RAMDISK
	isize = PGALIGN((uval)initrd_size);

	/* move ramdisk (if any) out of the way */
	if (!noinitrd) move_initrd();
#endif

	/* copy parameters where linux can find them */
	linux_parameters(cmdline);

	/* go .... */
	hprintf("\n");
	start_kernel(LOADADDR, 0, (unsigned) boot_params);
}
