/*
 * DM bswap16
 *
 * Copyright (C) 2012 glevand <geoffrey.levand@mail.ru>
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/device-mapper.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/bio.h>
#include <linux/slab.h>
#include <linux/mempool.h>
#include <linux/workqueue.h>

#define DM_MSG_PREFIX	"bswap16"

struct bswap16_c {
	struct dm_dev *dev;
	mempool_t *io_pool;
	struct workqueue_struct *io_queue;
};

struct bswap16_io {
	struct dm_target *target;
	struct work_struct work;
	struct bio *bio;
	int error;
	struct block_device *orig_bdev;
	bio_end_io_t *orig_end_io;
	void *orig_private;
};

static struct kmem_cache *bswap16_io_pool;

static void kbswap16d_queue_io(struct bswap16_io *io);

/*
 * bswap16_end_io
 */
static void bswap16_end_io(struct bio *bio, int error)
{
	struct bswap16_io *io = (struct bswap16_io *) bio->bi_private;
	struct bswap16_c *bc = (struct bswap16_c *) io->target->private;

	if (bio_data_dir(bio) == READ) {
		io->error = error;

		kbswap16d_queue_io(io);
	} else {
		bio->bi_bdev = io->orig_bdev;
		bio->bi_end_io = io->orig_end_io;
		bio->bi_private = io->orig_private;

		mempool_free(io, bc->io_pool);
		bio_endio(bio, error);
	}
}

/*
 * kbswap16d
 */
static void kbswap16d(struct work_struct *work)
{
	struct bswap16_io *io = container_of(work, struct bswap16_io, work);
	struct bswap16_c *bc = (struct bswap16_c *) io->target->private;
	struct bio *bio = io->bio;
	int error = io->error;
	struct bio_vec *bvec;
	char *buf, tmp;
	int i, j;

	if (!error) {
		bio_for_each_segment(bvec, bio, i) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,11,0)
			buf = __bio_kmap_atomic(bio, i, KM_USER0);
#else
			buf = __bio_kmap_atomic(bio, i);
#endif

			for (j = 0; j < bvec->bv_len; j += 2) {
				tmp = buf[j];
				buf[j] = buf[j + 1];
				buf[j + 1] = tmp;
			}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,11,0)
			__bio_kunmap_atomic(bio, KM_USER0);
#else
			__bio_kunmap_atomic(bio);
#endif
		}
	}

	if (bio_data_dir(bio) == READ) {
		bio->bi_bdev = io->orig_bdev;
		bio->bi_end_io = io->orig_end_io;
		bio->bi_private = io->orig_private;

		mempool_free(io, bc->io_pool);
		bio_endio(bio, error);
	} else {
		io->orig_bdev = bio->bi_bdev;
		io->orig_end_io = bio->bi_end_io;
		io->orig_private = bio->bi_private;

		bio->bi_bdev = bc->dev->bdev;
		bio->bi_end_io = bswap16_end_io;
		bio->bi_private = io;

		generic_make_request(bio);
	}
}

/*
 * kbswap16d_queue_io
 */
static void kbswap16d_queue_io(struct bswap16_io *io)
{
	struct bswap16_c *bc = (struct bswap16_c *) io->target->private;

	INIT_WORK(&io->work, kbswap16d);
	queue_work(bc->io_queue, &io->work);
}

/*
 * bswap16_io_alloc
 */
static struct bswap16_io *bswap16_io_alloc(struct dm_target *ti, struct bio *bio)
{
	struct bswap16_c *bc = (struct bswap16_c *) ti->private;
	struct bswap16_io *io;

	io = mempool_alloc(bc->io_pool, GFP_NOIO);
	io->target = ti;
	io->bio = bio;
	io->error = 0;

	return (io);
}

/*
 * bswap16_ctr
 */
static int bswap16_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
	struct bswap16_c *bc;
	int err;

	if (argc != 1) {
		ti->error = "Invalid argument count";
		return (-EINVAL);
	}

	bc = kmalloc(sizeof(*bc), GFP_KERNEL);
	if (!bc) {
		ti->error = "Cannot allocate context";
		return (-ENOMEM);
	}

	err = -EINVAL;

	if (dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &bc->dev)) {
		ti->error = "Device lookup failed";
		goto fail_free_context;
	}

	err = -ENOMEM;

	bc->io_pool = mempool_create_slab_pool(16, bswap16_io_pool);
	if (!bc->io_pool) {
		ti->error = "Couldn't allocate io mempool";
		goto fail_free_context;
	}

	err = -ENOMEM;

	bc->io_queue = alloc_workqueue("kbswap16d", WQ_NON_REENTRANT | WQ_MEM_RECLAIM, 1);
	if (!bc->io_queue) {
		ti->error = "Couldn't create io queue";
		goto fail_destroy_io_pool;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,9,0)
	ti->num_flush_requests = 1;
	ti->num_discard_requests = 1;
#else
	ti->num_flush_bios = 1;
	ti->num_discard_bios = 1;
#endif
	ti->private = bc;

	return (0);

fail_destroy_io_pool:

	mempool_destroy(bc->io_pool);

fail_free_context:

	kfree(bc);

	return (err);
}

/*
 * bswap16_dtr
 */
static void bswap16_dtr(struct dm_target *ti)
{
	struct bswap16_c *bc = (struct bswap16_c *) ti->private;

	ti->private = NULL;

	destroy_workqueue(bc->io_queue);
	mempool_destroy(bc->io_pool);
	dm_put_device(ti, bc->dev);
	kfree(bc);
}

/*
 * bswap16_map
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,8,0)
static int bswap16_map(struct dm_target *ti, struct bio *bio, union map_info *map_context)
#else
static int bswap16_map(struct dm_target *ti, struct bio *bio)
#endif
{
	struct bswap16_c *bc = (struct bswap16_c *) ti->private;
	struct bswap16_io *io;

	if (unlikely(bio->bi_rw & (REQ_FLUSH | REQ_DISCARD))) {
		bio->bi_bdev = bc->dev->bdev;
		return (DM_MAPIO_REMAPPED);
	}

	io = bswap16_io_alloc(ti, bio);

	if (bio_data_dir(bio) == READ) {
		io->orig_bdev = bio->bi_bdev;
		io->orig_end_io = bio->bi_end_io;
		io->orig_private = bio->bi_private;

		bio->bi_bdev = bc->dev->bdev;
		bio->bi_end_io = bswap16_end_io;
		bio->bi_private = io;

		generic_make_request(bio);
	} else {
		kbswap16d_queue_io(io);
	}

	return (DM_MAPIO_SUBMITTED);
}

static struct target_type bswap16_target = {
	.name		= "bswap16",
	.version	= { 1, 0, 0 },
	.module		= THIS_MODULE,
	.ctr		= bswap16_ctr,
	.dtr		= bswap16_dtr,
	.map		= bswap16_map,
};

/*
 * dm_bswap16_init
 */
static int __init dm_bswap16_init(void)
{
	int err;

	bswap16_io_pool = KMEM_CACHE(bswap16_io, 0);
	if (!bswap16_io_pool)
		return (-ENOMEM);

	err = dm_register_target(&bswap16_target);
	if (err < 0)
		DMERR("register failed %d", err);
 
	return (err);
}

/*
 * dm_bswap16_exit
 */
static void __exit dm_bswap16_exit(void)
{
	dm_unregister_target(&bswap16_target);
	kmem_cache_destroy(bswap16_io_pool);
}

module_init(dm_bswap16_init)
module_exit(dm_bswap16_exit)
 
MODULE_AUTHOR("glevand <geoffrey.levand@mail.ru>");
MODULE_DESCRIPTION(DM_NAME " target swapping bytes in each 16-bit word");
MODULE_LICENSE("GPL");
