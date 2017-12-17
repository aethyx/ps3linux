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
#include <types.h>
#include <bitmap.h>
#include <lib.h>
#include <objalloc.h>
#include <bitops.h>

uval
lbm_size(struct lbm *l)
{
	return sizeof (*l) + l->lbm_num * sizeof(uval);
}

struct lbm *
lbm_alloc(uval num_bits)
{
	uval size = sizeof(struct lbm) +
		(num_bits + UVAL_BITS - 1) / sizeof(uval);
	struct lbm *l = (struct lbm *)halloc(size);

	if (l == NULL) return NULL;

	return lbm_init(num_bits, size, l);
}

struct lbm *
lbm_init(uval num_bits, uval max_size, void* mem)
{
	struct lbm *l = (struct lbm *)mem;
	uval size = sizeof(struct lbm) +
		((num_bits + UVAL_BITS - 1) / UVAL_BITS) * sizeof(uval);
	if (max_size && size > max_size) {
		return NULL;
	}
	l->lbm_num = num_bits;
	return l;
}

void
lbm_set_all_bits(struct lbm *l, uval val)
{
	uval i = 0;
	uval words = (l->lbm_num + UVAL_BITS - 1) / UVAL_BITS;
	if (val) {
		val = ~((uval)0);
	}
	for (; i < words; ++i) {
		l->lbm_bits[i] = val;
	}
}

void
lbm_set_bits(struct lbm *l, uval start_bit, uval num_bits, uval val)
{
	if (start_bit + num_bits > l->lbm_num) {
		num_bits = l->lbm_num - start_bit;
	}
	while (num_bits) {
		uval start = (start_bit % UVAL_BITS);
		uval end = start + num_bits;

		/* set bits start .. end  to 1 */
		uval mask = ~((uval)0) << start;
		if (end >= UVAL_BITS) {
			end = UVAL_BITS;
		} else {
			mask ^= ~((uval)0) << end;
		}

		if (val) {
			l->lbm_bits[start_bit / UVAL_BITS] |= mask;
		} else {
			l->lbm_bits[start_bit / UVAL_BITS] &= ~mask;
		}
		start_bit += end - start;
		num_bits -= end - start;
	}
}


uval
lbm_find_range(struct lbm *l, uval val, uval num_bits, uval log_align,
	       uval base)
{
	uval i;
	uval words = (l->lbm_num + UVAL_BITS - 1) / UVAL_BITS;

	uval align = 1<<log_align;
	uval matched = 0;

	uval mask;
	uval mask_bits; // how many bits we'll check at a time
	i = 0;

	/* Alignment is set to base mod 1<<2 */
	i = i + base;
	/* align is always a power of 2 */
	i = (i + align - 1) & ~(align - 1);
	i = i - base;


	if (num_bits == 0) return LARGE_BITMAP_ERROR;
	while (i - matched <= ((words * UVAL_BITS) - num_bits)) {
		/* Fast check to see if entire block of bits is already used*/
		if ( ((val != 0) && l->lbm_bits[i / UVAL_BITS] == 0) ||
		     ((val == 0) && l->lbm_bits[i / UVAL_BITS] == ~0UL) ) {
			i+=UVAL_BITS;
			matched = 0;
			continue;
		}

		/* First matched bit must match alignment requirement */
		if (matched == 0) {
			/* Alignment is set to base mod 1<<2 */
			i = i + base;
			/* align is always a power of 2 */
			i = (i + align - 1) & ~(align - 1);
			i = i - base;
		}

		/* Test as many bits as possible in current word */
		if ((num_bits - matched) > (UVAL_BITS - (i % UVAL_BITS))) {
			mask_bits = UVAL_BITS - (i % UVAL_BITS);
		} else {
			mask_bits = num_bits - matched;
		}

		/* This forumulation necessary so it works when
		   mask_bits == UVAL_BITS */
		mask = (~((uval)0)) >> (UVAL_BITS - mask_bits);

		uval test = l->lbm_bits[i / UVAL_BITS] >> (i % UVAL_BITS);
		if ((val == 1 && ((test & mask) == mask))
		    || ((val == 0) && (test & mask) == ((uval)0))) {
			matched += mask_bits;

			if (matched == num_bits) {
				i += mask_bits;
				break;
			}

		} else {
			matched = 0;
		}

		if (matched) {
			i += mask_bits;
		} else {
			i += align;
		}
	}

	if (matched != num_bits) {
		return LARGE_BITMAP_ERROR;
	}

	return i - matched;
}

/*
 * Interfaces for using LBM's atmomically, great for tracking array
 * element usage.
 */
sval
lbm_acquire(struct lbm *l)
{
	uval bit;
	uval map;
	uval words;
	uval i;

	words = (l->lbm_num + UVAL_BITS - 1) / UVAL_BITS;
	for (i = 0; i < words; i++) {
		do {
			map = l->lbm_vbits[i];
			bit = ffz(map);
			if (bit < UVAL_BITS &&
			    ((i * UVAL_BITS) + bit) < l->lbm_num) {
				uval new;
				
				new = map | (1U << bit);
				if (cas_uval(&l->lbm_vbits[i], map, new)) {
					return ((i * UVAL_BITS) + bit);
				}
				/* find another bit in this word*/
				continue;
			}
			/* no valid bit, move onto next word */
		} while (0);
	}
	return -1;
}

sval
lbm_release(struct lbm *l, uval bit)
{
	uval i;
	uval b;
	uval new;
	uval map;

	if (bit >= l->lbm_num) {
		assert(0, "bit value out of range\n");
		return -1;
	}
	i = bit / UVAL_BITS;
	b = 1UL << (bit % UVAL_BITS);
	
	do {
		map = l->lbm_vbits[i];
		if (!(map & b)) {
			return -1;
		}

		new = map & ~b;
	} while (!cas_uval(&l->lbm_vbits[i], map, new));

	return bit;
}
