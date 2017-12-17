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
 * Arbitrary size bitmap.
 */

#ifndef _BITMAP_H
#define _BITMAP_H

#include <config.h>
#include <types.h>
#include <lib.h>

#define LARGE_BITMAP_ERROR (~((uval)0))

#define UVAL_BITS    (sizeof(uval) * 8)

/* LBM large bit map */
struct lbm {
	uval lbm_num;
	union {
		uval lbm_bit_array[0];
		uval __volatile__ lbm_vbit_array[0];
	} _lbm_arrays;
#define lbm_bits _lbm_arrays.lbm_bit_array
#define lbm_vbits _lbm_arrays.lbm_vbit_array
};

extern struct lbm *lbm_alloc(uval num_bits);

/* create num_bits sized bitmap at mem
 * bounded by max_size if non-zero
 */
extern struct lbm *lbm_init(uval num_bits, uval max_size, void *mem);
extern uval lbm_size(struct lbm *l);
extern void lbm_set_all_bits(struct lbm *l, uval val);
extern void lbm_set_bits(struct lbm *l,
			 uval start_bit, uval num_bits, uval val);

/* Locates num_bits bits set to "val" with an
   alignment ==  base mod 2^log_align */
extern uval lbm_find_range(struct lbm *l, uval val, uval num_bits,
			   uval log_align, uval align_base);

extern sval lbm_acquire(struct lbm *l);
extern sval lbm_release(struct lbm *l, uval bit);

static inline uval
test_bit(struct lbm *l, uval bit)
{
	assert(l->lbm_num > bit, "bit number exceeds bitmap size\n");
	return (l->lbm_bits[bit / UVAL_BITS] >> (bit % UVAL_BITS)) & 0x1;
}

static inline uval
test_bits(struct lbm *l, uval bit, uval num_bits, uval val)
{
	assert(l->lbm_num >= bit + num_bits,
	       "bit number exceeds bitmap size\n");
	while (bit < num_bits) {
		if (val) {
			if (!test_bit(l, bit))
				return 0;
		} else {
			if (test_bit(l, bit))
				return 0;
		}
		++bit;
	};
	return 1;
}

#endif /* ! _BITMAP_H */
