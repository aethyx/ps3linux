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

#ifndef _LLAN_H
#define _LLAN_H

#include <config.h>
#include <types.h>
#include <cpu.h>
#include <tce.h>
#include <mmu.h>
#include <resource.h>
#include <vio.h>

union llan_recvq_entry {
	uval64 lrqe_dwords[2];
	uval32 lrqe_words[4];

	/* *INDENT-OFF* */
	struct lrqe_bits_s {
		/* control fields */
		uval8 lrqe_vtoggle	: 1;
		uval8 lrqe_v		: 1;
		uval8 _lrqe_rev0	: 6;
		uval8 _lrqe_res1;
		uval16 lrqe_off;

		uval32 lrqe_len;
		uval64 lrqe_handle;
	} lrqe_bits;
	/* *INDENT-ON* */

} __attribute__ ((aligned(16)));

/* per llan structures */
#define LLAN_MEM_SZ 0x10000000

/* theoretically this be (LLAN_MEM_SZ >> LOG_PGSIZE) but hopefully we
 * can get away with the following value */
#define LLAN_DMA_WINDOW_SIZE	( 8 * 1024 * 1024 )
#define LLAN_BL_POOLS 		3	/* architected max is 256 */
#define LLAN_BL_POOL_NUM 	256	/* This is all linux wants */
#define LLAN_BL_SIZE 		PGSIZE
#define LLAN_BL_RECVQ		0
#define LLAN_BL_FILTER		1
#define LLAN_BL_POOL_START	2
#define LLAN_BL_POOL_END	(LLAN_BL_POOL_START + 256)
#define LLAN_BL_COUNTER	((LLAN_BL_SIZE / sizeof (union tce_bdesc)) - 1)

struct llan {
	uval64 ll_mac;
	struct tce_data ll_tce_data;
	union tce_bdesc ll_blist_pool[LLAN_BL_POOLS][LLAN_BL_POOL_NUM];
	/* end 64 bit quantaties */

	struct vios_resource ll_res[2];
	uval ll_owner_res;
	union llan_recvq_entry *ll_recvq;
	union tce_bdesc *ll_blist;
	uval64 *ll_mfilter;
	uval ll_size;
	uval ll_dma_size;
	struct os *ll_os;
	/* end ptr size quantaties */

	lock_t ll_lock;
	uval32 volatile ll_ipend;

	xirr_t ll_interrupt;
	uval8 ll_mcast_state;
	uval8 ll_registered;
	uval8 ll_recvq_toggle;
	__volatile__ uval8 ll_valid;

	sval16 ll_mac_idx;
	uval16 ll_mcast_count;
	uval16 ll_recvq_idx;
	uval16 ll_recvq_end;
};

#define LL_MCAST_RECV 0x1
#define LL_MCAST_FILTER 0x1
#define LL_MCAST_ENTRIES 255	/* 256? */

#define LMF_MODR	(1ULL << (63-44))
#define LMF_MODF	(1ULL << (63-45))
#define LMF_MODR_ALLOW	(1ULL << (63-46))
#define LMF_MODF_ALLOW	(1ULL << (63-47))
#define LMF_MODC_MASK	0x3ULL
#define LMF_MODC_NOMOD	0x0
#define LMF_MODC_ADD	0x1
#define LMF_MODC_REMOVE	0x2
#define LMF_MODC_CLEAR	0x3

#define LLAN_SEND_BD_NUM 6

extern uval llan_sys_init(void);
extern sval llan_register(struct os *os, uval uaddr,
			  uval bl, uval64 rq, uval fl, uval64 mac);
extern sval llan_free(struct os *os, uval uaddr);
extern sval llan_mcast_ctrl(struct cpu_thread *thread, uval uaddr,
			    uval64 fl, uval64 mmac);
extern sval llan_change_mac(struct os *os, uval uaddr, uval64 mac);
extern sval llan_add_buf(struct os *os, uval uaddr, union tce_bdesc bd);
extern sval llan_send(struct cpu_thread *thread, uval uaddr,
		      union tce_bdesc *bd, uval token);
extern sval llan_freebuf(struct os *os, uval uaddr, uval size);
#endif /* _LLAN_H */
