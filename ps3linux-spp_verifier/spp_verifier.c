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

/*
 * usage
 */
static void usage(const char *progname)
{
	fprintf(stderr, "usage: %s <self path> <profile path>\n", progname);
	exit(1);
}

/*
 * main
 */
int main(int argc, char **argv)
{
	const char *spuisofs_path;
	struct spe_shadow *shadow;
	struct spu_priv2 *priv2;
	struct spu_problem *problem;
	int err;

	if (argc != 3)
		usage(argv[0]);

	spuisofs_path = spuisofs_find();
	if (!spuisofs_path) {
		fprintf(stderr, "error: could not find spuisofs\n");
		return (1);
	}

	fprintf(stdout, "spuisofs found at %s\n", spuisofs_path);

	err = spufs_send_file(argv[1], spufs_make_path(spuisofs_path, SPUISOFS_APP_NAME));
	if (err) {
		fprintf(stderr, "error: could not send self\n");
		return (1);
	}

	err = spufs_send_file(argv[2], spufs_make_path(spuisofs_path, SPUISOFS_ARG1_NAME));
	if (err) {
		fprintf(stderr, "error: could not send profile\n");
		return (1);
	}

	err = spuisofs_run(spuisofs_path, 0x1050000003000001ull, spufs_file_length(argv[2]), 0);
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

	return (0);
}
