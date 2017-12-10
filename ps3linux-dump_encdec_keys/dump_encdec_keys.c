/*
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <spu.h>
#include <spufs.h>
#include <spuldrfs.h>

/*
 * usage
 */
static void usage(const char *progname)
{
	fprintf(stderr, "usage: %s <metldr path> <ldr path>\n", progname);
	exit(1);
}

/*
 * main
 */
int main(int argc, char **argv)
{
	const char *spuldrfs_path;
	uint64_t buf1_addr, buf2_addr;
	struct spe_shadow *shadow;
	struct spu_priv2 *priv2;
	struct spu_problem *problem;
	uint8_t *buf1;
	int err;

	if (argc != 3)
		usage(argv[0]);

	spuldrfs_path = spuldrfs_find();
	if (!spuldrfs_path) {
		fprintf(stderr, "error: could not find spuldrfs\n");
		return (1);
	}

	fprintf(stdout, "spuldrfs found at %s\n", spuldrfs_path);

	err = spufs_send_file(argv[1], spufs_make_path(spuldrfs_path, SPULDRFS_METLDR_NAME));
	if (err) {
		fprintf(stderr, "error: could not send metldr\n");
		return (1);
	}

	err = spufs_send_file(argv[2], spufs_make_path(spuldrfs_path, SPULDRFS_LDR_NAME));
	if (err) {
		fprintf(stderr, "error: could not send ldr\n");
		return (1);
	}

	buf1_addr = spufs_get_kern_virt_addr(spufs_make_path(spuldrfs_path, SPULDRFS_INFO_NAME), SPULDRFS_BUF1_NAME);
	if (!buf1_addr) {
		fprintf(stderr, "error: could not get buf1 kernel virtual address\n");
		return (1);
	}

	fprintf(stderr, "buf1 kernel virtual address %llx\n", buf1_addr);

	buf2_addr = spufs_get_kern_virt_addr(spufs_make_path(spuldrfs_path, SPULDRFS_INFO_NAME), SPULDRFS_BUF2_NAME);
	if (!buf2_addr) {
		fprintf(stderr, "error: could not get buf2 kernel virtual address\n");
		return (1);
	}

	fprintf(stderr, "buf2 kernel virtual address %llx\n", buf2_addr);

	err = spuldrfs_run(spuldrfs_path);
	if (err) {
		fprintf(stderr, "error: could not run\n");
		return (1);
	}

	shadow = (struct spe_shadow *) spufs_mmap_file(spufs_make_path(spuldrfs_path, SPULDRFS_SHADOW_NAME),
		sizeof(struct spe_shadow), O_RDONLY | O_SYNC);
	if (!shadow) {
		fprintf(stderr, "error: could not map shadow\n");
		return (1);
	}

	priv2 = (struct spu_priv2 *) spufs_mmap_file(spufs_make_path(spuldrfs_path, SPULDRFS_PRIV2_NAME),
		sizeof(struct spu_priv2), O_RDONLY | O_SYNC);
	if (!priv2) {
		fprintf(stderr, "error: could not map priv2\n");
		return (1);
	}

	problem = (struct spu_problem *) spufs_mmap_file(spufs_make_path(spuldrfs_path, SPULDRFS_PROBLEM_NAME),
		sizeof(struct spu_problem), O_RDWR | O_SYNC);
	if (!problem) {
		fprintf(stderr, "error: could not map problem\n");
		return (1);
	}

	buf1 = (uint8_t *) spufs_mmap_file(spufs_make_path(spuldrfs_path, SPULDRFS_BUF1_NAME),
		4096, O_RDWR | O_SYNC);
	if (!buf1) {
		fprintf(stderr, "error: could not map buf1\n");
		return (1);
	}

	while (in_be64(&priv2->puint_mb_R) != 1)
		usleep(100000);

	fprintf(stdout, "priv2: puint_mb_R %llx\n", in_be64(&priv2->puint_mb_R));

	while (in_be32(&problem->pu_mb_R) != 1)
		usleep(100000);

	fprintf(stdout, "problem: pu_mb_R %x\n", in_be32(&problem->pu_mb_R));

	while (in_be64(&priv2->puint_mb_R) != 0x666)
		usleep(100000);

	fprintf(stdout, "priv2: puint_mb_R %llx\n", in_be64(&priv2->puint_mb_R));

	*(uint64_t *) buf1 = buf2_addr;

	err = spu_mfc_dma_xfer(problem, 0x3e000, buf1_addr, sizeof(uint64_t), 0x7, 0, MFC_GET_CMD);
	if (err) {
		fprintf(stderr, "error: could not do MFC DMA\n");
		return (1);
	}

	spu_mfc_dma_wait(problem, 0x7);

	out_be32(&problem->spu_mb_W, 0x666);

	while (in_be32(&problem->spu_status_R) & SPU_STATUS_RUNNING)
		usleep(100000);

	fprintf(stdout, "problem: spu_status_R %x\n", in_be32(&problem->spu_status_R));

	return (0);
}
