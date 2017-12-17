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
#include <lpar.h>
#include <llan.h>
#include <os.h>
#include <mmu.h>
#include <pmm.h>
#include <util.h>
#include <xir.h>
#include <tce.h>
#include <vio.h>
#include <objalloc.h>
#include <debug.h>

static __inline__ struct vios_resource *
llan_getvres(struct llan *l)
{
	return &l->ll_res[l->ll_owner_res];
}

static __inline__ struct sys_resource *
llan_getsres(struct llan *l)
{
	struct vios_resource *res;

	res = llan_getvres(l);
	
	return &res->vr_res;
}

static __inline__ struct os *
llan_getos(struct llan *l)
{
	struct vios_resource *res;

	res = llan_getvres(l);
	
	return res->vr_res.sr_owner;
}

#define MACS_PER_PORT 8
struct llan_ports_s {
	uval64 volatile lm_mac;	/* The Registered MAC */
	uval64 lm_cache[MACS_PER_PORT];
	struct llan *volatile lm_lan;	/* yes the pointer */
	uval lm_next;		/* could be a uval16 */
};

#define LLAN_IFACES XIRR_DEVID_SZ
static struct llan_ports_s llan_ports[LLAN_IFACES];
static const uval64 bcast = 0xffffffffffffULL;

static struct llan *
llan_mac_find(uval64 mac, uval *in_cache)
{
	sval16 i;

	for (i = 0; i < LLAN_IFACES; i++) {
		if (llan_ports[i].lm_mac != 0) {
			if (llan_ports[i].lm_mac == mac) {
				assert(llan_ports[i].lm_lan != NULL,
				       "mac mismatch\n");
				*in_cache = 0;
				return llan_ports[i].lm_lan;
			} else {
				/* scan the cache */
				int c;

				for (c = 0; c < MACS_PER_PORT; c++) {
					if (llan_ports[i].lm_cache[c] == mac) {
						assert((llan_ports[i].lm_lan !=
							NULL),
						       "mac mismatch\n");
						*in_cache = 1;
						return llan_ports[i].lm_lan;
					}
				}
			}
		}
	}
	return NULL;
}

static struct llan_ports_s *
llan_mac_lookup_lan(struct llan *l)
{
	return &llan_ports[l->ll_mac_idx];
}

static int
llan_mac_cache_add(struct llan *l, uval64 mac)
{
	struct llan_ports_s *m = llan_mac_lookup_lan(l);

	if (m == NULL) {
		return -1;
	}
	if (m->lm_mac != 0ULL && m->lm_lan == l) {
		int c;

		/* look for an empty spot */
		for (c = 0; c < MACS_PER_PORT; c++) {
			if (m->lm_cache[c] == mac) {
				return c;
			}
			if (m->lm_cache[c] == 0) {
				m->lm_cache[c] = mac;
				m->lm_next = (c + 1) % MACS_PER_PORT;
				return c;
			}
		}
		/*
		 * cache is full, use the one after last, When we
		 * start removing, this should be rethought
		 */
		m->lm_cache[m->lm_next] = mac;
		m->lm_next = (m->lm_next + 1) % MACS_PER_PORT;
	}
	return -1;
}

static void
llan_mac_cache_free(struct llan *l, uval64 mac)
{
	struct llan_ports_s *m = llan_mac_lookup_lan(l);

	if (m != NULL) {
		if (m->lm_mac != 0ULL && m->lm_lan == l) {
			int c;

			/* find it in cache */
			for (c = 0; c < MACS_PER_PORT; c++) {
				if (m->lm_cache[c] == mac) {
					m->lm_cache[c] = 0;
					m->lm_next = c;
					return;
				}
			}
		}
	}
}

static sval16
llan_mac_add(struct llan *l)
{
	sval16 i;
	struct llan *d;
	uval in_cache;

	/* look for dup */
	d = llan_mac_find(l->ll_mac, &in_cache);
	if (d != NULL) {
		/* FIXME: is this either necessary OR correct? */
		/* mac already in use */
		if (!in_cache) {
			return -1;
		} else {
			llan_mac_cache_free(d, l->ll_mac);
		}
	}

	/* find first empty */
	for (i = 0; i < LLAN_IFACES; i++) {
		/* we make sure that if a mac address is found then
		 * the lan ptr must be valid */
		if (cas_uval((volatile uval *)&llan_ports[i].lm_lan,
			     0, (uval)l)) {
			assert(llan_ports[i].lm_mac == 0, "mac should be 0\n");
			llan_ports[i].lm_mac = l->ll_mac;
			return i;
		}
	}
	return -1;
}

static sval16
llan_mac_change(struct llan *l, uval64 mac)
{
	sval16 i;

	/* find old */
	for (i = 0; i < LLAN_IFACES; i++) {
		if (llan_ports[i].lm_lan == l) {
			assert(llan_ports[i].lm_mac == l->ll_mac,
			       "mac mismatch\n");
			llan_ports[i].lm_mac = mac;
			return i;
		}
	}
	return -1;
}

static int
llan_mac_free(struct llan *l)
{
	sval16 i = l->ll_mac_idx;

	if (cas_uval64(&llan_ports[i].lm_mac, l->ll_mac, 0)) {
		llan_ports[i].lm_lan = NULL;
		memset(&llan_ports[i].lm_cache, 0,
		       sizeof (llan_ports[i].lm_cache));
		return 1;
	}
	assert(0, "unexpected mac value\n");
	return 0;
}

static int
llan_vlan_hdr(struct llan *l, uval16 vh)
{
	l = l;
	vh = vh;
	return 1;
}

static inline struct llan *
llan_get(uval32 liobn)
{
	return (struct llan *)xir_get_device(liobn);
}

static sval
llan_tce_put(struct os *os, uval32 liobn, uval ioba, union tce ltce)
{
	struct llan *l;

	l = llan_get(liobn);
	if (l == NULL) {
		return H_Parameter;
	}
	if (ioba > (l->ll_size - PGSIZE)) {
		return H_Parameter;
	}

	assert(l->ll_tce_data.t_tce != NULL, "TCE was not set up!\n");

	return tce_put(os, &l->ll_tce_data, ioba, ltce);
}

static void *
llan_bd_tce_xlate(struct llan *l, union tce_bdesc bd)
{
	return tce_bd_xlate(&l->ll_tce_data, bd);
}

static union tce_bdesc *
llan_bdesc(struct llan *l, uval sz)
{
	if (sz == 0) {
		return NULL;
	}
	assert((sz & 0x3ffULL) == 0, "size is not a multiple of 1k\n");
	sz = sz >> 10;

	/* so 1k size should be located at LLAN_BL_POOL */
	sz += LLAN_BL_POOL_START - 1;
	if (sz >= LLAN_BL_COUNTER) {
		return NULL;
	}
	return &l->ll_blist[sz];
}

static sval
llan_register_locked(struct llan *l, uval bl, uval64 rq, uval fl, uval64 mac)
{
	union tce_bdesc bd;
	union tce_bdesc *pool;
	uval pg;

	assert(l->ll_tce_data.t_tce != NULL, "TCE was not set up!\n");
	
	if (bl != ALIGN_DOWN(bl, PGSIZE)) {
		return H_Parameter;
	}
	pg = bl >> LOG_PGSIZE;
	if (!(pg < l->ll_tce_data.t_entries &&
	      l->ll_tce_data.t_tce[pg].tce_bits.tce_read == 1 &&
	      l->ll_tce_data.t_tce[pg].tce_bits.tce_write == 1)) {
		return H_Parameter;
	}
	pg = l->ll_tce_data.t_tce[pg].tce_bits.tce_rpn << LOG_PGSIZE;
	l->ll_blist = (union tce_bdesc *)pg;

	bd.lbd_dword = rq;
	assert(bd.lbd_bits.lbd_ctrl_v, "recvq bufdesc is not valid");
	l->ll_recvq = llan_bd_tce_xlate(l, bd);
	if (l->ll_recvq == NULL) {
		return H_Parameter;
	}
	if ((bd.lbd_bits.lbd_len % sizeof (l->ll_recvq[0])) != 0) {
		return H_Parameter;
	}
	l->ll_recvq_end = bd.lbd_bits.lbd_len / sizeof (l->ll_recvq[0]);
	l->ll_recvq_toggle = 1;
	l->ll_recvq_idx = 0;

	/* The RPA says the driver should zero the recvq, but Linux is
	 * not so we will do it */
	zero_mem(l->ll_recvq, bd.lbd_bits.lbd_len);

	if (fl != ALIGN_DOWN(fl, PGSIZE)) {
		return H_Parameter;
	}
	pg = fl >> LOG_PGSIZE;
	if (!(pg < l->ll_tce_data.t_entries &&
	      l->ll_tce_data.t_tce[pg].tce_bits.tce_read == 1 &&
	      l->ll_tce_data.t_tce[pg].tce_bits.tce_write == 1)) {
		return H_Parameter;
	}
	pg = l->ll_tce_data.t_tce[pg].tce_bits.tce_rpn << LOG_PGSIZE;
	l->ll_mfilter = (void *)pg;

	if (l->ll_mac != mac) {
		DEBUG_OUT(DBG_LLAN,
			  "%s: updating mac address, "
			  "from: 0x%llx to: 0x%llx\n",
			  __func__, l->ll_mac, mac);
		l->ll_mac = mac;
	}

	/* initialize the buffer list */
	zero_mem(l->ll_blist, LLAN_BL_SIZE);
	copy_out(&l->ll_blist[LLAN_BL_RECVQ], &bd, sizeof (bd));

	/* need to make one for the mcast filter, reuse all other bd bits */
	bd.lbd_bits.lbd_addr = fl;
	bd.lbd_bits.lbd_len = LL_MCAST_ENTRIES * sizeof (*l->ll_mfilter);
	copy_out(&l->ll_blist[LLAN_BL_FILTER], &bd, sizeof (bd));

	for (pg = 0; pg < LLAN_BL_POOLS; pg++) {
		/* linux only wants 2k, 4k and 10k buffer pools */
		static const uval32 psz[] = {
			1024 * 2,
			1024 * 4,
			1024 * 10
		};

		pool = llan_bdesc(l, psz[pg]);
		assert(pool != NULL, "cannot init blist\n");

		{
			union tce_bdesc lbd;

			lbd.lbd_dword = 0;
			lbd.lbd_bits.lbd_ctrl_v = 1;
			lbd.lbd_bits.lbd_len = psz[pg];
			lbd.lbd_bits.lbd_addr = pg;
			copy_out(pool, &lbd, sizeof (lbd));
		}
	}

	/* make sure the receive buffer list pools are clear */
	memset(l->ll_blist_pool, 0, sizeof (l->ll_blist_pool));

	l->ll_mac_idx = llan_mac_add(l);
	if (l->ll_mac_idx == -1) {
		assert(0, "can't run out\n");
		return H_Hardware;
	}

	/*
	 * Ok everything checks out.. update registered
	 */
	l->ll_registered = 1;
	return H_Success;
}

sval
llan_register(struct os *os, uval uaddr,
	      uval bl, uval64 rq, uval fl, uval64 mac)
{
	struct llan *l;

	l = llan_get(uaddr);

	if (l == NULL) {
		return H_Parameter;
	}

	if (llan_getos(l) != os) {
		return H_Parameter;
	}

	if (!l->ll_valid) {
		return H_Parameter;
	}

	if (lock_tryacquire(&l->ll_lock)) {
		sval ret;

		if (l->ll_registered) {
			DEBUG_OUT(DBG_LLAN,
				  "%s: lan 0x%lx is already registered\n",
				  __func__, uaddr);
			return H_Hardware;
		}

		ret = llan_register_locked(l, bl, rq, fl, mac);

		lock_release(&l->ll_lock);
		xir_default_config(l->ll_interrupt, &os->cpu[0]->thread[0], l);
		return ret;
	}

	DEBUG_OUT(DBG_LLAN, "%s: lan 0x%lx is locked\n", __func__, uaddr);

	return H_Parameter;
}

sval
llan_free(struct os *os, uval uaddr)
{
	struct llan *l;
	sval ret;

	l = llan_get(uaddr);

	if (l == NULL) {
		return H_Parameter;
	}
	assert(llan_getos(l) == os, "check failed\n");

	if (lock_tryacquire(&l->ll_lock)) {
		if (l->ll_registered) {
			llan_mac_free(l);
			l->ll_registered = 0;
			ret = H_Success;
		} else {
			DEBUG_OUT(DBG_LLAN,
				  "%s: lan 0x%lx is not registered\n",
				  __func__, uaddr);
			ret = H_Parameter;
		}
		lock_release(&l->ll_lock);
	} else {
		DEBUG_OUT(DBG_LLAN,
			  "%s: lan 0x%lx is locked\n", __func__, uaddr);
		ret = H_Hardware;
	}
	return ret;
}

static sval
llan_mcast_ctrl_locked(struct llan *l, uval64 *state_p, uval64 fl, uval64 mmac)
{
	uval32 modc;
	sval ret = H_Success;

	/* don't know why we cannot ignore reserved bits, but the spec
	 * says to complain */
	if (fl & ~(LMF_MODR | LMF_MODR_ALLOW |
		   LMF_MODF | LMF_MODF_ALLOW | LMF_MODC_MASK)) {
		return H_Parameter;
	}

	if (fl & LMF_MODR) {
		if (fl & LMF_MODR_ALLOW) {
			l->ll_mcast_state |= LL_MCAST_RECV;
		} else {
			l->ll_mcast_state &= ~(LL_MCAST_RECV);
		}
	}
	if (fl & LMF_MODF) {
		if (fl & LMF_MODF_ALLOW) {
			l->ll_mcast_state |= LL_MCAST_FILTER;
		} else {
			l->ll_mcast_state &= ~(LL_MCAST_FILTER);
		}
	}

	modc = fl & LMF_MODC_MASK;
	switch (modc) {
	default:
		assert(0, "should not get here\n");
		return H_Parameter;
		break;

	case LMF_MODC_NOMOD:
		/* do nothing */
		break;

	case LMF_MODC_ADD:
		/* add */
		/* should search first and silently succeed dups
		 * without incrementing */
		if (l->ll_mcast_count < LL_MCAST_ENTRIES) {
			mmac = mmac;
			l->ll_mcast_count += 1;
		} else {
			ret = H_Constrained;
		}
		break;
	case LMF_MODC_REMOVE:
		/* search for mmac */
		if (l->ll_mcast_count > 0) {
			mmac = mmac;
			l->ll_mcast_count -= 1;
		} else {
			ret = H_Not_Found;
		}
		break;
	case LMF_MODC_CLEAR:
		/* clear all */
		zero_mem(l->ll_mfilter, PGSIZE);
		l->ll_mcast_count = 0;
		break;
	}

	*state_p = l->ll_mcast_count;
	if (l->ll_mcast_state & LL_MCAST_RECV) {
		*state_p |= LMF_MODR_ALLOW;
	}
	if (l->ll_mcast_state & LL_MCAST_FILTER) {
		*state_p |= LMF_MODF_ALLOW;
	}

	return ret;
}

sval
llan_mcast_ctrl(struct cpu_thread *thread, uval uaddr, uval64 fl, uval64 mmac)
{
	struct llan *l;

	l = llan_get(uaddr);

	if (l == NULL) {
		return H_Parameter;
	}
	assert(llan_getos(l) == thread->cpu->os, "check failed\n");

	if (lock_tryacquire(&l->ll_lock)) {
		uval64 state;
		sval ret = llan_mcast_ctrl_locked(l, &state, fl, mmac);

		lock_release(&l->ll_lock);

		return_arg(thread, 1, state);
		return ret;
	}

	DEBUG_OUT(DBG_LLAN, "%s: lan 0x%lx is locked\n", __func__, uaddr);

	return H_Parameter;
}

sval
llan_change_mac(struct os *os, uval uaddr, uval64 mac)
{
	struct llan *l;
	sval ret;

	l = llan_get(uaddr);

	if (l == NULL) {
		return H_Parameter;
	}
	assert(llan_getos(l) == os, "check failed\n");

	if (lock_tryacquire(&l->ll_lock)) {
		if (l->ll_registered) {
			sval16 i;

			i = llan_mac_change(l, mac);
			assert(i > 0, "mac not found\n");
			/* may be at a new index */
			l->ll_mac = mac;
			l->ll_mac_idx = i;
			ret = H_Success;
		} else {
			ret = H_Parameter;
		}
		lock_release(&l->ll_lock);
	} else {
		DEBUG_OUT(DBG_LLAN,
			  "%s: lan 0x%lx is locked\n", __func__, uaddr);
		ret = H_Hardware;
	}
	return ret;
}

sval
llan_add_buf(struct os *os, uval uaddr, union tce_bdesc bd)
{
	struct llan *l;

	assert(bd.lbd_bits.lbd_len >= 1024, "bd_len too small\n");

	l = llan_get(uaddr);

	if (l == NULL) {
		return H_Parameter;
	}
	assert(llan_getos(l) == os, "check failed\n");

	if (lock_tryacquire(&l->ll_lock)) {
		void *bd_addr;

		assert(bd.lbd_bits.lbd_ctrl_v, "bufdesc is not valid");

		bd_addr = llan_bd_tce_xlate(l, bd);

		if (bd_addr != NULL) {
			int i;
			union tce_bdesc *pool;

			/* check for the buffer list for a pool descriptor */
			pool = llan_bdesc(l, bd.lbd_bits.lbd_len);

			/* FIXME: this is probably true */
			assert(pool != NULL,
			       "simple pool indexing is insufficient\n");

			struct lbd_bits_s lbd_bits;

			copy_in(&lbd_bits, pool, sizeof (lbd_bits));

			assert(lbd_bits.lbd_ctrl_v == 1,
			       "unexpected blist pool\n");

			assert(lbd_bits.lbd_len == bd.lbd_bits.lbd_len,
			       "incorrect pool\n");

			/* addr is index into the blist_pool */
			pool = &l->ll_blist_pool[lbd_bits.lbd_addr][0];

			for (i = 0; i < LLAN_BL_POOL_NUM; i++) {
				/* FIXME: way to simple right now */
				if (pool[i].lbd_bits.lbd_ctrl_v == 0) {
					pool[i] = bd;
					break;
				}
			}
			lock_release(&l->ll_lock);

			if (i < LLAN_BL_POOL_NUM) {
				return H_Success;
			}
			return H_Resource;
		}
		return H_Parameter;
	}
	DEBUG_OUT(DBG_LLAN, "%s: lan 0x%lx is locked\n", __func__, uaddr);
	return H_Hardware;
}

static void *
llan_getbuf(struct llan *l, uval size)
{
	int b;
	int p;

	/* first find the right pool */
	for (p = LLAN_BL_POOL_START; p < LLAN_BL_POOL_END; p++) {
		struct lbd_bits_s lbd_bits;

		copy_in(&lbd_bits, &l->ll_blist[p], sizeof (lbd_bits));
		if (lbd_bits.lbd_len >= size) {
			p = lbd_bits.lbd_addr;
			for (b = 0; b < LLAN_BL_POOL_NUM; b++) {
				union tce_bdesc bd;
				union tce_bdesc volatile *pool;

				pool = &l->ll_blist_pool[p][0];

				/* FIXME: finer grained lock needed here */
				bd = pool[b];
				if (bd.lbd_bits.lbd_ctrl_v) {
					bd.lbd_bits.lbd_ctrl_v = 0;
					pool[b] = bd;

					return llan_bd_tce_xlate(l, bd);
				}
			}
			/* FIXME: Do we only consider the first pool
			 * big enough? */
			break;
		}
	}

	DEBUG_OUT(DBG_LLAN, "%s: did not find a big enough pool\n", __func__);

	{
		uval64 w;

		copy_in(&w, &l->ll_blist[LLAN_BL_COUNTER].lbd_dword,
			sizeof (w));
		w++;
		copy_out(&l->ll_blist[LLAN_BL_COUNTER].lbd_dword, &w,
			 sizeof (w));
	}

	return NULL;
}

static void
llan_write_rqe(struct llan *l, union llan_recvq_entry *rq, void *rb)
{
	union llan_recvq_entry *rqp;

	rqp = &l->ll_recvq[l->ll_recvq_idx];

	rq->lrqe_bits.lrqe_vtoggle = l->ll_recvq_toggle & 0x1;

	l->ll_recvq_idx += 1;
	if (l->ll_recvq_idx >= l->ll_recvq_end) {
		l->ll_recvq_idx = 0;
		l->ll_recvq_toggle += 1;
	}

	/* clear the mark dword which makes it invalid */
	/* then write the receive queue entry in the correct order */
	{
		uval i;

		copy_in(&rq->lrqe_bits.lrqe_handle, rb, sizeof (uval64));
		zero_mem(&rqp->lrqe_words[0], sizeof (uval));
		copy_out(&rqp->lrqe_dwords[1], &rq->lrqe_dwords[1],
			 sizeof (uval64));

		i = sizeof (uval64) / sizeof (uval);;
		do {
			--i;
			copy_out(&rqp->lrqe_words[i], &rq->lrqe_words[i],
				 sizeof (uval));
		} while (i > 0);
	}
}

sval
llan_freebuf(struct os *os, uval uaddr, uval size)
{
	struct llan *l;
	union llan_recvq_entry rq;
	void *rb;

	l = llan_get(uaddr);

	if (l == NULL) {
		return H_Parameter;
	}
	assert(llan_getos(l) == os, "check failed\n");

	/* find a buffer of the proper size */
	rb = llan_getbuf(l, size);
	if (rb == NULL) {
		return H_Not_Found;
	}

	/* build a new recv queue entry */
	rq.lrqe_words[0] = 0;
	rq.lrqe_bits.lrqe_len = size;

	/* inform the OS of the free buffer */
	llan_write_rqe(l, &rq, rb);

	return H_Success;
}

static sval
llan_enqueue(struct llan *l, uval end, uval sum,
	     union tce_bdesc *bd, void **pa)
{
	union llan_recvq_entry rq;
	void *rb;
	uval i;
	char *p;
	uval align;

	/* Must at least cover the opaque word */
#if CACHE_LINE_SIZE < 8
#error "CACHE_LINE_SIZE is not defined"
#endif
	align = CACHE_LINE_SIZE + ((uval)pa[0] & (CACHE_LINE_SIZE - 1));

	rb = llan_getbuf(l, sum + align);	/* includes the opaque word */
	if (rb == NULL) {
		return H_Busy;
	}

	/* build a new recv queue entry */
	rq.lrqe_words[0] = 0;
	rq.lrqe_bits.lrqe_v = 1;
	rq.lrqe_bits.lrqe_off = align;
	rq.lrqe_bits.lrqe_len = sum;

	/* copy the bits */
	p = rb;
	p += rq.lrqe_bits.lrqe_off;
	for (i = 0; i < end; i++) {
		uval sz = bd[i].lbd_bits.lbd_len;

		copy_mem(p, pa[i], sz);
		p += sz;
	}

	/* inform the OS of the received frame */
	llan_write_rqe(l, &rq, rb);

	xir_raise(l->ll_interrupt, NULL);

	return H_Success;
}

static sval
llan_broadcast(struct llan *l, uval end, uval sum,
	       union tce_bdesc *bd, void **pa)
{
	int i;
	sval ret = H_Success;

	for (i = 0; i < LLAN_IFACES; i++) {
		struct llan_ports_s *m = &llan_ports[i];

		if (m->lm_mac != 0ULL && m->lm_mac != l->ll_mac) {
			/* not self */
			struct llan *d = m->lm_lan;
			sval lret;

			if ((d->ll_mcast_state & LL_MCAST_RECV) == 0) {
				continue;
			}
#ifdef LLAN_FILTER_ENABLE
			/* no filtering yet */
			if ((d->ll_mcast_state & LL_MCAST_FILTER) == 0) {
				/* check filter */
			}
#endif
			lret = llan_enqueue(d, end, sum, bd, pa);

			if (lret != H_Success)
				ret = lret;
		}
	}
	return ret;
}

static uval
llan_send_locked(struct llan *l, union tce_bdesc *bd, uval *token)
{
	uval sum = 0;
	uval sz;
	uval end = 0;
	void *pa[LLAN_SEND_BD_NUM];
	uval64 dst;
	uval64 src;
	uval16 vh;
	unsigned char *ptr;
	int i = 0;

	assert(*token == 0, "Cannot handle continuations yet\n");

	/* FIXME: doing this in a very simple way for now */
	do {
		sz = bd[i].lbd_bits.lbd_len;
		if (sz > 0) {
			assert(bd[i].lbd_bits.lbd_ctrl_v,
			       "bufdesc is not valid");
			pa[i] = llan_bd_tce_xlate(l, bd[i]);
			if (pa[i] == NULL) {
				return H_Parameter;
			}
			sum += sz;
			++end;
		}
		++i;
	} while (sz > 0 && i < LLAN_SEND_BD_NUM);

	if (end == 0) {
		/* first bd is empty */
		return H_Parameter;
	}
	if (sum > l->ll_size) {
		return H_Parameter;
	}

	/* code assumes first fragment contains at least the
	 * destination and source MAC addresses and optional VLAN
	 * header */
	assert(bd[0].lbd_bits.lbd_len > 13, "first fragment is too small\n");

	{
		unsigned char hdr[14];

		ptr = (unsigned char *)copy_in(hdr, pa[0], sizeof (hdr));
	}

	/* first six bytes are the destination MAC */
	dst = ((uval64)ptr[5]);
	dst |= ((uval64)ptr[4]) << 8;
	dst |= ((uval64)ptr[3]) << 16;
	dst |= ((uval64)ptr[2]) << 24;
	dst |= ((uval64)ptr[1]) << 32;
	dst |= ((uval64)ptr[0]) << 40;

	/* next six bytes are the source MAC */
	src = ((uval64)ptr[11]);
	src |= ((uval64)ptr[10]) << 8;
	src |= ((uval64)ptr[9]) << 16;
	src |= ((uval64)ptr[8]) << 24;
	src |= ((uval64)ptr[7]) << 32;
	src |= ((uval64)ptr[6]) << 40;
	llan_mac_cache_add(l, src);

	vh = ((uval)ptr[13]);
	vh |= ((uval)ptr[12]) << 8;
	llan_vlan_hdr(l, vh);

	if (dst != bcast) {
		struct llan *ld;
		uval dummy;

		ld = llan_mac_find(dst, &dummy);
		if (ld != NULL) {
			return llan_enqueue(ld, end, sum, bd, pa);
		}
		/* send packet on all ports */
	}
	return llan_broadcast(l, end, sum, bd, pa);
}

sval
llan_send(struct cpu_thread *thread, uval uaddr,
	  union tce_bdesc *bd, uval token)
{
	struct llan *l;

	l = llan_get(uaddr);

	if (l == NULL) {
		return H_Parameter;
	}
	assert(llan_getos(l) == thread->cpu->os, "check failed\n");

	if (lock_tryacquire(&l->ll_lock)) {
		sval ret = llan_send_locked(l, bd, &token);

		lock_release(&l->ll_lock);

		return_arg(thread, 1, token);
		return ret;
	}

	DEBUG_OUT(DBG_LLAN, "%s: lan 0x%lx is locked\n", __func__, uaddr);

	return H_Parameter;
}

static struct llan *
llan_acquire_internal(uval dma_range)
{
	struct llan *l;

	l = halloc(sizeof (*l));
	if (l != NULL) {
		sval isrc;
		uval xenc;

		isrc = xir_find(XIRR_CLASS_LLAN);
		assert(isrc != -1, "Can't register llan\n");
		xenc = xirr_encode(isrc, XIRR_CLASS_LLAN);

		if (isrc >= 0) {
			memset(l, 0, sizeof (*l));

			/* setup tce areas */
			dma_range = tce_alloc(&l->ll_tce_data, 0x0, dma_range);
			if (dma_range > 0) {
				lock_init(&l->ll_lock);

				/* local address tag */
				l->ll_mac = 0x02ULL << 40;
				l->ll_mac |= xenc;
				l->ll_size = LLAN_MEM_SZ;
				l->ll_interrupt = xenc;
				l->ll_ipend = 0;
				l->ll_owner_res = 0;
				/* both get the same liobn */
				l->ll_res[0].vr_liobn = xenc;
				l->ll_res[1].vr_liobn = xenc;
				l->ll_valid = 1;
				l->ll_dma_size = dma_range;

				DEBUG_OUT(DBG_LLAN,
					  "%s: mac: 0x%llx, xirr: 0x%x\n",
					  __func__, l->ll_mac,
					  l->ll_interrupt);

				struct sys_resource *res = llan_getsres(l);
				resource_init(res, NULL, INTR_SRC);


				return l;
			}
			tce_free(&l->ll_tce_data);
		}
		xir_default_config(xenc, NULL, NULL);
	}
	hfree(l, sizeof(*l));
	return NULL;
}

static sval
llan_acquire(struct cpu_thread *thread, uval dma_range)
{
	struct llan *l;

	l = llan_acquire_internal(dma_range);
	if (l) {
		struct sys_resource *res = llan_getsres(l);
		struct vios_resource *lres = llan_getvres(l);
		sval rc;
		uval dum;

		xir_default_config(l->ll_interrupt, thread, l);

		rc = insert_resource(res, thread->cpu->os);
		assert(rc >= 0,
		       "Can't give l-lan: 0x%lx to os\n", lres->vr_liobn);

		lock_acquire(&res->sr_lock);
		rc = accept_locked_resource(res, &dum);
		assert(rc == H_Success, "Can't bind llan to os\n");
		lock_release(&res->sr_lock);

		return_arg(thread, 1, lres->vr_liobn);
		return_arg(thread, 2, 0);
		return_arg(thread, 3, l->ll_dma_size);
		return H_Success;
	}
	return H_UNAVAIL;
}

	

static sval
llan_release(struct os *os, uval32 liobn)
{
	struct llan *l;

	l = llan_get(liobn);

	if (l == NULL) {
		return H_Parameter;
	}
	if (llan_getos(l) != os) {
		return H_Parameter;
	}

	llan_free(os, liobn);
	
	tce_free(&l->ll_tce_data);

	xir_default_config(l->ll_interrupt, NULL, NULL);

	hfree(l, sizeof (*l));
			
	return H_Success;
}

static sval
llan_grant(struct sys_resource **res,
	   struct os *src, struct os *dst, uval liobn)
{
	struct llan *l;
	struct cpu_thread *thread = &dst->cpu[0]->thread[0];
	uval old;
	uval new;

	l = llan_get(liobn);

	if (l == NULL) {
		return H_Parameter;
	}

	assert(llan_getos(l) == src, "check failed\n");

	old = l->ll_owner_res;
	new = old ^ 1; /* flip between 0 and 1 */

	assert(l->ll_res[new].vr_res.sr_owner == NULL, "new owner not 0\n");
	if (l->ll_res[new].vr_res.sr_owner == NULL) {

		/* disable interrupts */
		xir_default_config(l->ll_interrupt, NULL, NULL);

		l->ll_owner_res = new;
		l->ll_res[new].vr_res.sr_owner = dst;
		*res = &l->ll_res[new].vr_res;

		resource_init(*res, NULL, INTR_SRC);

		tce_ia(&l->ll_tce_data);
		force_rescind_resource(&l->ll_res[old].vr_res);

		xir_default_config(l->ll_interrupt, thread, l);

		return H_Success;
	}
	return H_UNAVAIL;
}

static sval
llan_accept(struct os *os, uval liobn, uval *retval)
{
	struct llan *l;

	l = llan_get(liobn);

	if (l != NULL) {
		if (os == llan_getos(l)) {
			*retval = liobn;
			return H_Success;
		}
	}
	return H_Parameter;
}

static sval
llan_invalidate(struct os *os, uval liobn)
{
	struct llan *l;

	l = llan_get(liobn);

	if (l == NULL) {
		return H_Parameter;
	}
	if (os == llan_getos(l)) {
		l->ll_valid = 0;
	}
	return H_Success;
}

static sval
llan_return(struct os *os, uval liobn)
{
	struct llan *l;

	l = llan_get(liobn);

	if (l == NULL) {
		return H_Parameter;
	}
	if (os == llan_getos(l)) {
		DEBUG_OUT(DBG_LLAN, "%s: owner\n", __func__);
	}
	return H_Success;
}

static sval
llan_rescind(struct os *os, uval liobn)
{
	struct llan *l;

	l = llan_get(liobn);

	if (l == NULL) {
		return H_Parameter;
	}

	if (os == llan_getos(l)) {
		uval other = l->ll_owner_res ^ 1;

		if (l->ll_res[other].vr_res.sr_owner != NULL) {
			force_rescind_resource(&l->ll_res[other].vr_res);
		}
		return llan_release(os, liobn);
	}
	/* just allow the rescind to happen */
	return H_Success;
}

static sval
llan_conflict(struct sys_resource *res1,
	      struct sys_resource *res2,
	      uval liobn)
{
	struct llan *l;

	l = llan_get(liobn);

	if (l == NULL) {
		return H_Parameter;
	}
	(void)res1; (void)res2;
	assert(0, "what do we do here?\n");

	return H_Success;
}


static struct vios llan_vios = {
	.vs_name = "llan",
	.vs_tce_put = llan_tce_put,
	.vs_acquire = llan_acquire,
	.vs_release = llan_release,
	.vs_grant = llan_grant,
	.vs_accept = llan_accept,
	.vs_invalidate = llan_invalidate,
	.vs_return = llan_return,
	.vs_rescind = llan_rescind,
	.vs_conflict = llan_conflict
};

uval
llan_sys_init(void)
{
	if (vio_register(&llan_vios, HVIO_LLAN) == HVIO_LLAN) {
		xir_init_class(XIRR_CLASS_LLAN, NULL, NULL);
		return 1;
	}
	assert(0, "failed to register: %s\n", llan_vios.vs_name);
	return 0;
}
