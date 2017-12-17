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

#include <tce.h>
#include <llan.h>
#include <logical.h>
#include <lpar.h>
#include <os.h>
#include <pgalloc.h>
#include <pmm.h>
#include <hype.h>

//#define TCE_DEBUG

static inline uval
tce_calc_entries(uval dma_window_size)
{
	return ((dma_window_size + (PGSIZE - 1)) / PGSIZE);
}

static inline uval
tce_calc_pages(uval dma_window_size)
{
	return ((tce_calc_entries(dma_window_size) *
		 sizeof (union tce)) + (PGSIZE - 1)) / PGSIZE;
}

void
tce_ia(struct tce_data *tce_data)
{
	uval size = tce_data->t_entries * sizeof (tce_data->t_tce[0]);
	memset(tce_data->t_tce, 0, size);
}

uval
tce_alloc(struct tce_data *tce_data, uval base, uval dma_window_size)
{
	uval entries = tce_calc_entries(dma_window_size);
	uval size = tce_calc_pages(dma_window_size) * PGSIZE;

	tce_data->t_tce = (union tce *)get_pages(&phys_pa, size);
	if (NULL != tce_data->t_tce) {
		memset(tce_data->t_tce, 0, size);
		tce_data->t_entries = entries;
		tce_data->t_base = base;
		tce_data->t_alloc_size = size;
		return dma_window_size;
	}
	return 0;
}

void
tce_free(struct tce_data *tce_data)
{
	assert(NULL != tce_data, "tce data is NULL?!");
	assert(NULL != tce_data->t_tce, "t_data is NULL!");
	free_pages(&phys_pa, (uval)tce_data->t_tce, tce_data->t_alloc_size);
	tce_data->t_entries = 0;
	tce_data->t_base = 0;
	tce_data->t_alloc_size = 0;
	tce_data->t_tce = NULL;
}

sval
tce_put(struct os *os, struct tce_data *tce_data, uval ioba, union tce ltce)
{
	int pg;
	uval la;
	uval pa;
	volatile union tce *ptce;
	union tce *tce;
	int entries;

	assert(tce_data != NULL, "tce_data must not be NULL!\n");

	tce = tce_data->t_tce;
	entries = tce_data->t_entries;

	pg = ioba >> LOG_PGSIZE;
	assert(pg < entries, "TCE index is too large\n");
	if (pg >= entries) {
		return H_Parameter;
	}
	ptce = &tce[pg];

	la = ltce.tce_bits.tce_rpn << LOG_PGSIZE;
	pa = logical_to_physical_address(os, la, PGSIZE);
	if (pa == INVALID_PHYSICAL_ADDRESS) {
		return H_Parameter;
	}
#ifdef TCE_DEBUG
	hprintf("%s: ioba=0x%lx la=0x%lx pa=0x%lx\n", __func__, ioba, la, pa);
#endif
	ltce.tce_bits.tce_rpn = pa >> LOG_PGSIZE;

	/* FIXME: how do quiesce traffic on the fake TCE?  if there is
	 * something in flight */
	/* needs to occur atomically, we don;t care what was there before */

	ptce->tce_dword = ltce.tce_dword;

	return H_Success;
}

void *
tce_bd_xlate(struct tce_data *tce_data, union tce_bdesc bd)
{
	uval pg;
	uval s = bd.lbd_bits.lbd_addr;
	uval sz = bd.lbd_bits.lbd_len;
	uval ep;
	uval bytes;
	union tce *tce;
	uval entries;

	assert(tce_data != NULL, "tce_data must not be NULL!\n");

	tce = tce_data->t_tce;
	entries = tce_data->t_entries;

	pg = s >> LOG_PGSIZE;
	bytes = s - ALIGN_DOWN(s, PGSIZE);

	ep = ALIGN_UP(s + sz, PGSIZE) >> LOG_PGSIZE;
	s = ALIGN_DOWN(s, PGSIZE) >> LOG_PGSIZE;

	/* make sure all consecutive pages are represented */
	while (s < ep) {
		uval rw;

		if (s >= entries) {
			return NULL;
		}
		rw = tce[s].tce_bits.tce_read < 1;
		rw |= tce[s].tce_bits.tce_write;
		switch (rw) {
		case 0:
			return NULL;
			break;

#ifdef TCE_DEBUG
		case 1:
			hprintf("%s: tce WO\n", __func__);
			break;
		case 2:
			hprintf("%s: tce RO\n", __func__);
			break;
#endif
		case 3:
		default:
			break;
		}
		++s;
		/* FIXME: do I need to check if the buffer crosses a
		 * chunk boundary? */
	}

	pg = (tce[pg].tce_bits.tce_rpn << LOG_PGSIZE) + bytes;
	return (void *)pg;
}
