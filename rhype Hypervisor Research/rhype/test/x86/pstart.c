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

#include <test.h>
#include <loadFile.h>
#include <ofd.h>
#include <util.h>
#include <mmu.h>

#define OS_LINK_BASE 0x100000

int
start_partition(struct partition_status* ps, struct load_file_entry* lf,
		uval ofd)
{	sval rc;
	uval lofd = 0;
	uval esi;
	lofd = ofd_lpar_create(ps, lofd, ofd);
	
	esi = prom_load(ps, lofd);

	rc = hcall_start(NULL, ps->lpid, lf->va_entry, lf->va_tocPtr,
			 0, esi, 0, 0, 0);
	assert(rc == H_Success, "hcall_start() failed\n");
	return 1;
}

struct chunk {
	uval from;
	uval to;
	uval len;
};

struct run_info {
	struct chunk chunks[10];
	uval entry;
	uval parms;
};

static uval
get_chunks(struct load_file_entry *lf, struct chunk *chunks, uval *numchunks)
{
	uval highest = 0;
	uval i = 0;
	uval segno = lf->numberOfLoaderSegments;
	uval base = lf->va_entry - OS_LINK_BASE;

	for (i = 0; i < segno; i++) {
		struct loader_segment_entry *le = &lf->loaderSegmentTable[i];
		uval isize = le->file_size;
		uval offset = le->va_start;

		chunks[i].from = base + offset;
		chunks[i].to   = offset;
		chunks[i].len  = isize;

		hprintf("chunk %ld: from=0x%08lx - 0x%08lx to "
		        "0x%08lx - 0x%08lx  [0x%lx]\n",
		        i,
		        chunks[i].from,
		        chunks[i].from + chunks[i].len,
		        chunks[i].to,
		        chunks[i].to + chunks[i].len,
		        chunks[i].len);

		if (chunks[i].from + chunks[i].len > highest) {
			highest = chunks[i].from + chunks[i].len;
		}
	}
	chunks[i].len = 0;
	
	*numchunks = i;
	return highest;
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
		: "0" (count/4), 
		  "q" (count), 
		  "1" ((long)dest), 
		  "2" ((long)src)
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
	        :"a" (0),
	         "1" (dest),
	         "0" (count)
	        :"memory");
}


/*
 * This relocator code copies chunks of linux code to their
 * proper place and then jumps into the linux code.
 * The relocator code itself will be copied to a place where
 * it is save to run.
 */
static void
relocator(struct run_info * ri)
{
	uval i = 0;
	while (ri->chunks[i].len) {
		uval from = ri->chunks[i].from;
		uval to   = ri->chunks[i].to;

		move((void *)to, 
		     (void *)from, 
		     ri->chunks[i].len);
		
		if (ri->chunks[i+1].len && 1 == i) {
			clear((void *)(to+ri->chunks[i].len),
			      ri->chunks[i+1].to - (to + ri->chunks[i].len));
		}
		
		i++;
	}
	/*
	 * Jump to our entry along with all the kerne
	 * parameters in %esi
	 */
	asm volatile ("movl %1, %%esi\n\t"
	              "movl %0, %%eax\n\t"
	              "jmp  *%%eax\n\t"
	              :
	              : "r" (ri->entry),
	                "r" (ri->parms)
		      : "eax", "esi");
}

int
restart_partition(struct partition_status* ps, struct load_file_entry* lf,
		  uval ofd)
{
	uval lofd = 0;
	uval esi;

	lofd = ofd_lpar_create(ps, lofd, ofd);

	esi = prom_load(ps, lofd);

	rrupts_off();

	/*
	 * most of the time I will have to relocate the code.
	 */
	if (OS_LINK_BASE != lf->va_entry) {
		uval addr;
		uval esp;
		void (*reloc)(struct run_info *ri);
		struct run_info ri;
		uval new_ri;
		uval numchunks;
		uval magic;
		/* 
		 * The address of the lboot entry plus the parameters to 
		 * pass to it
		 */
		ri.entry = OS_LINK_BASE;
		ri.parms = esi;

		hputs("\n\nNeed to relocate OS!\n");
		addr = get_chunks(lf, ri.chunks, &numchunks);
		addr = (addr + 0xff) & 0xffffff00UL;

		/*
		 * The injected partition info also needs to 
		 * move
		 */
		magic = lf->loaderSegmentTable[0].file_start;
		magic += 0x10;

		if (*(uval64 *)magic == HYPE_PARTITION_INFO_MAGIC_NUMBER) {
			uval offset = lf->va_entry - OS_LINK_BASE;
			uval partinfo = *((uval*)(magic + sizeof(uval64)));

			ri.chunks[numchunks].from = partinfo + offset;
			ri.chunks[numchunks].to   = partinfo;
			ri.chunks[numchunks].len  = sizeof(pinfo);
			
			ri.chunks[numchunks+1].len = 0;
		}

		hprintf("chunk %ld: from=0x%08lx - 0x%08lx to "
		        "0x%08lx - 0x%08lx  [0x%lx]\n",
		        numchunks,
		        ri.chunks[numchunks].from,
		        ri.chunks[numchunks].from + ri.chunks[numchunks].len,
		        ri.chunks[numchunks].to,
		        ri.chunks[numchunks].to + ri.chunks[numchunks].len,
		        ri.chunks[numchunks].len);

		hprintf("Copying relocator to 0x%lx\n",addr);
		memcpy((void *)addr, relocator, 0x100);

		new_ri = addr + 0x100;
		hprintf("putting run_info [%d] information to 0x%lx\n",
		        sizeof(ri),
		        new_ri);
		memcpy((void *)new_ri, &ri, sizeof(ri));

		/* 
		 * We need a new stack pointer since the current
		 * stack is somewhere where we will copy the OS
		 * into.
		 * Put the new stack pointer after the run_info 
		 * structure.
		 */
		esp = addr + 0x200;
		hprintf("New stack pointer at 0x%lx\n",
		        esp);
		reloc = (void *)addr;

		asm volatile ("movl  %0, %%esp\n\t"
		              "movl  %1, %%eax\n\t"
		              "pushl %%eax\n\t"
		              "movl  %2, %%eax\n\t"
		              "call  *%%eax\n\t"
		              :
		              : "r" (esp),
		                "r" (new_ri),
		                "r" (reloc));
	} else {
		asm volatile ("movl %1, %%esi\n\t"
		              "movl %0, %%eax\n\t"
		              "call *%%eax\n\t"
		              :
		              : "r" (lf->va_entry),
		                "r" (esi)
			      : "eax", "esi");
	}

	/* should never get here ... */
	return 1;	
}
