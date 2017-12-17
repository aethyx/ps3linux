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
 * place for atomic operations and simple locks.
 */

#ifndef _GENERIC_ATOMIC_H
#define _GENERIC_ATOMIC_H

#include <config.h>


static inline uval
atomic_add(volatile uval *value, uval add)
{
	uval old;

	do {
		old = *value;
	} while (!cas_uval(value, old, old + add));
	return old + add;
}

static inline uval32
atomic_add32(uval32 *value, uval32 add)
{
	uval32 old;

	do {
		old = *value;
	} while (!cas_uval32(value, old, old + add));
	return old + add;
}

static inline uval
cas_ptr(volatile void *ptr, void *oval, void *nval)
{
	return cas_uval((volatile uval *)ptr, (uval)oval, (uval)nval);
}

/*
 * We pass pointers around for no other reason then to allow for
 * better locks later.
 */
typedef volatile uval32 lock_t;
static lock_t const lock_unlocked = 0;

static inline uval
lock_held(lock_t *lock)
{
	return ((*lock) != lock_unlocked);
}

static inline void
lock_init(lock_t *lock)
{
	*lock = lock_unlocked;
}

static inline void
lock_acquire(lock_t *lock)
{
	while (!cas_uval32(lock, lock_unlocked, ~lock_unlocked)) {
		continue;
	}
	sync_after_acquire();
}

static inline int
lock_tryacquire(lock_t *lock)
{
	int ret = 0;

	if (cas_uval32(lock, lock_unlocked, ~lock_unlocked)) {
		ret = 1;
	}
	sync_after_acquire();
	return ret;
}

static inline void
lock_release(lock_t *lock)
{
	sync_before_release();
	*lock = lock_unlocked;
}

typedef volatile uval32 rw_lock_t;

static inline void
rw_lock_init(rw_lock_t *lock)
{
	*lock = 0;
}

/* locked by write */
static inline uval32
write_locked(rw_lock_t *lock)
{
	return (1 << 31) & *lock;
}

/* locked by read */
static inline uval32
read_locked(rw_lock_t *lock)
{
	return (*lock) & ((1UL << 31) - 1);
}

static inline void
read_lock_acquire(rw_lock_t *lock)
{
	uval32 val;

	/* Lock is acquired if we can increment lower 31 bits, while
	 * uppermost bit is 0. */
	do {
		val = *lock & ((1UL << 31) - 1);
	} while (!cas_uval32(lock, val, val + 1));
	sync_after_acquire();
}

static inline void
write_lock_acquire(rw_lock_t *lock)
{
	/* Lock is acquired if we can set 32nd bit, while all other
	 * bits are 0 */
	while (!cas_uval32(lock, 0, 1 << 31)) {
		continue;
	}
	sync_after_acquire();
}

static inline void
write_lock_release(rw_lock_t *lock)
{
	sync_before_release();
	*lock = 0;
}

static inline void
read_lock_release(rw_lock_t *lock)
{
	uval32 val;

	/* We want to decrement the low-order 31-bits atomically */
	sync_before_release();
	do {
		val = *lock;
	} while (!cas_uval32(lock, val, val - 1));

	/* necessary? */
	sync_after_acquire();
}


/* These operations assume that we're trying to use a single bit
 * in a 32 bit uval as a lock bit.
 * "val" is the bitmask representing this bit */
static inline uval32
lockbit(uval32 *value, uval32 val)
{
	uval32 old;
	do {
		old = *value;
		old &= ~val;

		/* Now atomically ensure the bit is set */
	} while (!cas_uval32(value, old, old | val));
	sync_after_acquire();
	return old;
}

static inline uval32
unlockbit(uval32 *value, uval32 val)
{
	uval32 old;
	sync_before_release();
	do {
		old = *value;
		old &= ~val;

		/* Now atomically ensure the bit is cleared */
	} while (!cas_uval32(value, old | val, old));
	return old;
}


#endif /* ! _GENERIC_ATOMIC_H */
