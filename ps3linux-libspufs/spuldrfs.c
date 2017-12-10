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

#include "spuldrfs.h"

#define PROC_MOUNTS	"/proc/mounts"

/*
 * spuldrfs_find
 */
const char *spuldrfs_find(void)
{
	static char path[1024];
	char dev[1024], type[1024], opts[1024];
	int dump, pass;
	int n;
	FILE *fp;

	fp = fopen(PROC_MOUNTS, "r");
	if (!fp)
		return (NULL);

	while (1) {
		n = fscanf(fp, "%s %s %s %s %d %d\n", dev, path, type, opts, &dump, &pass);
		if (n == EOF)
			break;
		else if (n != 6)
			continue;

		if (!strcmp(type, "spuldrfs")) {
			fclose(fp);
			return (path);
		}
	}

	fclose(fp);

	return (NULL);
}

/*
 * spuldrfs_run
 */
int spuldrfs_run(const char *path)
{
	int fd;
	char buf[4096];
	int dummy, nwritten;

	sprintf(buf, "%s/%s", path, SPULDRFS_RUN_NAME);

	fd = open(buf, O_WRONLY);
	if (fd < 0)
		return (-1);

	nwritten = write(fd, &dummy, sizeof(dummy));
	if (nwritten != sizeof(dummy))
		goto fail_close_file;

	close(fd);

	return (0);

fail_close_file:

	close(fd);

	return (-1);
}
