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
#include <spuisofs.h>

struct spu_arg1 {
	union {
		struct {
			uint64_t dbgbuf_addr;
			uint64_t dbgbuf_length;
			uint32_t param;
		} in;

		struct {
			uint8_t data[0x10];
		} out;

		uint8_t pad[0x80];
	} u;

	uint8_t dbgbuf[0x800];
};

/*
 * usage
 */
static void usage(const char *progname)
{
	fprintf(stderr, "usage: %s <self path> <eid0 path> <param>\n", progname);
	exit(1);
}

/*
 * main
 */
int main(int argc, char **argv)
{
	const char *spuisofs_path;
	uint32_t param;
	char *endptr;
	struct spu_arg1 *spu_arg1;
	uint64_t arg1_addr;
	struct spe_shadow *shadow;
	struct spu_priv2 *priv2;
	struct spu_problem *problem;
	int err;

	if (argc != 4)
		usage(argv[0]);

	spuisofs_path = spuisofs_find();
	if (!spuisofs_path) {
		fprintf(stderr, "error: could not find spuisofs\n");
		return (1);
	}

	fprintf(stdout, "spuisofs found at %s\n", spuisofs_path);

	param = strtoul(argv[3], &endptr, 0);
	if (*endptr != '\0')
		usage(argv[0]);

	err = spufs_send_file(argv[1], spufs_make_path(spuisofs_path, SPUISOFS_APP_NAME));
	if (err) {
		fprintf(stderr, "error: could not send self\n");
		return (1);
	}

	err = spufs_send_file(argv[2], spufs_make_path(spuisofs_path, SPUISOFS_ARG2_NAME));
	if (err) {
		fprintf(stderr, "error: could not send eid0\n");
		return (1);
	}

	arg1_addr = spufs_get_kern_virt_addr(spufs_make_path(spuisofs_path, SPUISOFS_INFO_NAME), SPUISOFS_ARG1_NAME);
	if (!arg1_addr) {
		fprintf(stderr, "error: could not get arg1 kernel virtual address\n");
		return (1);
	}

	fprintf(stderr, "arg1 kernel virtual address %llx\n", arg1_addr);

	spu_arg1 = (struct spu_arg1 *) spufs_mmap_file(spufs_make_path(spuisofs_path, SPUISOFS_ARG1_NAME),
		sizeof(struct spu_arg1), O_RDWR | O_SYNC);
	if (!spu_arg1) {
		fprintf(stderr, "error: could not map arg1\n");
		return (1);
	}

	memset(spu_arg1, 0, sizeof(struct spu_arg1));
	spu_arg1->u.in.dbgbuf_addr = arg1_addr + offsetof(struct spu_arg1, dbgbuf);
	spu_arg1->u.in.dbgbuf_length = sizeof(spu_arg1->dbgbuf);
	spu_arg1->u.in.param = param;

	err = spuisofs_run(spuisofs_path, 0x1050000003000001ull, 0x80, spufs_file_length(argv[2]));
	if (err) {
		fprintf(stderr, "error: could not run\n");
		return (1);
	}

	shadow = (struct spe_shadow *) spufs_mmap_file(spufs_make_path(spuisofs_path, SPUISOFS_SHADOW_NAME),
		sizeof(struct spe_shadow), O_RDONLY | O_SYNC);
	if (!shadow) {
		fprintf(stderr, "error: could not map shadow\n");
		return (1);
	}

	priv2 = (struct spu_priv2 *) spufs_mmap_file(spufs_make_path(spuisofs_path, SPUISOFS_PRIV2_NAME),
		sizeof(struct spu_priv2), O_RDONLY | O_SYNC);
	if (!priv2) {
		fprintf(stderr, "error: could not map priv2\n");
		return (1);
	}

	problem = (struct spu_problem *) spufs_mmap_file(spufs_make_path(spuisofs_path, SPUISOFS_PROBLEM_NAME),
		sizeof(struct spu_problem), O_RDWR | O_SYNC);
	if (!problem) {
		fprintf(stderr, "error: could not map problem\n");
		return (1);
	}

	while (in_be64(&shadow->spe_execution_status) != 7)
		usleep(100000);

	fprintf(stdout, "shadow: spe_execution_status %llx\n", in_be64(&shadow->spe_execution_status));

	fprintf(stdout, "priv2: puint_mb_R %llx\n", in_be64(&priv2->puint_mb_R));

	err = spuisofs_cont(spuisofs_path);
	if (err) {
		fprintf(stderr, "error: could not continue\n");
		return (1);
	}

	while (in_be64(&shadow->spe_execution_status) != 11)
		usleep(100000);

	fprintf(stdout, "shadow: spe_execution_status %llx\n", in_be64(&shadow->spe_execution_status));

	fprintf(stdout, "problem: spu_status_R %x\n", in_be32(&problem->spu_status_R));

	if ((in_be32(&problem->spu_status_R) & 0xffff0000) == 0x1000000) {
		fprintf(stdout, "result:\n");
		spufs_hex_fprintf(stdout, spu_arg1->u.out.data, sizeof(spu_arg1->u.out.data));
	}

	return (0);
}
