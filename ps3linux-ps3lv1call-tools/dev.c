/*-
 * Copyright (C) 2012 glevand <geoffrey.levand@mail.ru>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
 *    without modification, immediately at the beginning of the file.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/*
 * dev_do_lv1call
 */
int dev_do_lv1call(const char *device, uint64_t num_lv1call,
    uint64_t *in_args, int num_in_args, uint64_t *out_args, int64_t *result)
{
	int fd;
	uint64_t buf[9];
	int len, n;

	if (num_in_args > 8)
		return (-1);

	fd = open(device, O_RDWR);
	if (fd < 0)
		return (-1);

	buf[0] = num_lv1call;
	memcpy(buf + 1, in_args, num_in_args * sizeof(uint64_t));

	len = (1 + num_in_args) * sizeof(uint64_t);

	n = write(fd, buf, len);
	if (n != len) {
		close(fd);
		return (-1);
	}

	n = read(fd, buf, sizeof(buf));
	if (n < sizeof(int64_t)) {
		close(fd);
		return (-1);
	}

	close(fd);

	n = (n - sizeof(int64_t)) / sizeof(uint64_t);

	*result = (int64_t) buf[0];
	memcpy(out_args, buf + 1, n * sizeof(uint64_t));

	return (n);
}
