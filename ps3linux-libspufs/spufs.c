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
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include "spufs.h"

/*
 * spufs_make_path
 */
const char *spufs_make_path(const char *dir, const char *file)
{
	static char path[4096];

	sprintf(path, "%s/%s", dir, file);

	return (path);
}

/*
 * spufs_get_kern_virt_addr
 */
uint64_t spufs_get_kern_virt_addr(const char *path, const char *name)
{
	uint64_t addr;
	char buf[4096];
	int n;
	FILE *fp;

	fp = fopen(path, "r");
	if (!fp)
		return (0);

	while (1) {
		n = fscanf(fp, "%s %llx\n", buf, &addr);
		if (n == EOF)
			break;
		else if (n != 2)
			continue;

		if (!strcmp(name, buf)) {
			fclose(fp);
			return (addr);
		}
	}

	fclose(fp);

	return (0);
}

/*
 * spufs_send_file
 */
int spufs_send_file(const char *src_path, const char *dst_path)
{
	int src_fd, dst_fd;
	char buf[4096];
	int nread, nwritten;

	src_fd = open(src_path, O_RDONLY);
	if (src_fd < 0)
		return (-1);

	dst_fd = open(dst_path, O_WRONLY);
	if (dst_fd < 0)
		goto fail_close_src;

	while (1) {
		nread = read(src_fd, buf, sizeof(buf));
		if (nread < 0)
			goto fail_close_dst;
		else if (nread == 0)
			break;

		nwritten = write(dst_fd, buf, nread);
		if (nwritten != nread)
			goto fail_close_dst;
	}

	close(src_fd);
	close(dst_fd);

	return (0);

fail_close_dst:

	close(dst_fd);

fail_close_src:

	close(src_fd);

	return (-1);
}

/*
 * spufs_mmap_file
 */
char *spufs_mmap_file(const char *path, size_t len, int flags)
{
	int fd, prot;
	char *map;

	fd = open(path, flags);
	if (fd < 0)
		return (NULL);

	prot = PROT_READ;
	if (flags & O_RDWR)
		prot |= PROT_WRITE;

	map = mmap(0, len, prot, MAP_SHARED, fd, 0);
	if (map == (void *) MAP_FAILED)
		map = NULL;

	close(fd);

	return (map);
}

/*
 * spufs_file_length
 */
int spufs_file_length(const char *path)
{
	struct stat st;
	int err;

	err = stat(path, &st);
	if (err)
		return (-1);

	return (st.st_size);
}

/*
 * spufs_hex_fprintf
 */
void spufs_hex_fprintf(FILE *fp, const unsigned char *buf, size_t len)
{
	int i;

	if (len <= 0)
		return;

	for (i = 0; i < len; i++) {
		if ((i > 0) && !(i % 16))
			fprintf(fp, "\n");

		fprintf(fp, "%02x ", buf[i]);
	}

	fprintf(fp, "\n");
}
