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

#ifndef _TEST_H
#define _TEST_H

#include <config.h>
#include <asm.h>
#include <lpar.h>
#include <types.h>
#include <lib.h>
#include <hcall.h>
#include <partition.h>

struct partition_status {
	uval lpid;
	uval init_mem;
	uval init_mem_size;
	uval vterm;
	const char *name;
	uval slot;
	uval32 active;
	uval32 msgrcv;
	uval log_htab_bytes;
};

extern uval log_htab_bytes;

#define MAX_MANAGED_PARTITIONS 23
extern struct partition_status partitions[];

extern uval rags_state(void) __attribute__ ((weak));
extern void rags_ask(void) __attribute__ ((weak));
extern uval rags_register(struct partition_status *ps, uval ofd);

extern uval register_partition(struct partition_status *ps, uval ofd);
extern struct partition_info pinfo[2];
extern uval hcall_cons_init(uval ofd);
extern uval iohost_lpid;
extern uval test_os(uval argc, uval argv[]);
extern void vterm_service(struct partition_status *ps, uval sz);
extern uval vterm_create(struct partition_status *ps,
			 uval *server, uval *client);
extern uval vterm_create_server(void);

extern uval llan_create(uval dma_size, uval lpid, uval *liobn);

struct crq_maintenance {
	uval   cm_slot;
	uval   cm_crq_type;
	uval   cm_dma_sz;
	uval32 cm_owner_server; /* lpid of server */
	uval32 cm_owner_client; /* lpid of client */
	uval32 cm_liobn_server;
	uval32 cm_liobn_client;
};

enum {
	ROLE_SERVER,
	ROLE_CLIENT
};

struct crq_maintenance *crq_create(uval lpid, uval role, uval dma_sz);
void crq_release(uval32 lpid);

/*
 * break this out later
 */
extern uval _text_start;	/* Only allowed to take the address of this! */
extern uval _vec_start;		/* Only allowed to take the address of this! */
extern char __bss_start[];
extern uval _partition_info;

/* for the decrementer tests */
extern uval decr_cnt;

/* This functions called from asm */
extern uval aipc_handler(uval ex, uval *regs);
extern void assert_at_0(void) __attribute__ ((noreturn));

/* these only exist for test OSes */
extern sval hprintf_nlk(const char *fmt, ...)
	__attribute__ ((format(printf, 1, 2), no_instrument_function));
extern sval vhprintf_nlk(const char *fmt, va_list ap)
	__attribute__ ((no_instrument_function));
extern void hputs_nlk(const char *buf)
	__attribute__ ((no_instrument_function));

#ifdef DEBUG
extern void ex_stack_assert(void) __attribute__ ((noreturn));
#endif

/* Convert a virtual address to OS-physical.  Assumes that "testvec"
 * is at the beginning of the binary.
 */
#define V2P(addr) (((uval)addr) - (uval)&_text_start)

#define PGSIZE_LG_1 0x1000000
#define SELECT_LG_1 0
#define PGSIZE_LG_2 0x10000
#define SELECT_LG_2 1

#define PGSIZE_LG PGSIZE_LG_1
#define SELECT_LG SELECT_LG_1

/*
 * Some helper functions and macros.
 */

static inline uval
yield(uval how)
{
	if (how > 0) {
		uval rc = H_Success;

		while (how-- > 0 && rc == H_Success) {
			rc = hcall_yield(NULL, H_SELF_LPID);
		}
		return rc;
	}
	for (;;) {
		hcall_yield(NULL, H_SELF_LPID);
	}
}

static inline uval
get16chars(char *buf)
{
	uval ret_vals[(16 / sizeof (uval)) + 1];
	uval rc;

	do {
		yield(1);
		rc = hcall_get_term_char(ret_vals, 0);
		assert(rc == H_Success, "hcall_get_term_char: failed\n");
	} while (ret_vals[0] == 0);

	memcpy(buf, (char *)&ret_vals[1], ret_vals[0]);

	return ret_vals[0];
}

extern sval hcall_put_term_string(uval channel, uval count, const char *str);
extern sval hcall_get_term_string(int channel, char str[16]);


struct load_file_entry;
extern int start_partition(struct partition_status *ps,
			   struct load_file_entry *lf, uval ofd);

extern int restart_partition(struct partition_status *ps,
			     struct load_file_entry *lf, uval ofd);

extern uval map_pages(uval raddr, uval eaddr, uval size);

extern uval load_in_lpar(struct partition_status *ps,
			 uval istart, uval isize, uval offset);

extern uval launch_image(uval init_mem, uval init_mem_size,
			 const char *name, uval ofd);

extern uval reload_image(const char *name, uval ofd);

extern void updatePartInfo(uval action, int lpid);

/* This is for our memory */
extern struct pg_alloc pa;

/* This is for the system-wide logical-address space */
extern struct pg_alloc logical_pa;

#ifdef USE_LIBBZ2
#include <pgalloc.h>
extern sval dcomp_img(char *in_start, uval in_size,
		      char **out_start, uval *mem_size, uval *data_size,
		      struct pg_alloc *pa);
#endif
#endif /* _TEST_H */
