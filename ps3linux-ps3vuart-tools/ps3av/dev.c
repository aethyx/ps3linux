/*-
 * Copyright (C) 2011, 2012 glevand <geoffrey.levand@mail.ru>
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

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "ps3av_msg.h"
#include "dev.h"

static int fd = -1;

int
dev_open(const char *device)
{
	if (fd >= 0)
		close(fd);

	fd = open(device, O_RDWR);
	if (fd < 0)
		return (-1);

	return (0);
}

void
dev_close(void)
{
	if (fd >= 0)
		close(fd);
}

int
dev_write(const void *buf, size_t len)
{
	if (fd < 0)
		return (-1);

	return (write(fd, buf, len));
}

int
dev_read(void *buf, size_t len)
{
	char tmp[4096];
	struct ps3av_reply *reply;
	int ret;

	if (fd < 0)
		return (-1);

	reply = (struct ps3av_reply *) tmp;

	while (1) {
		ret = read(fd, &reply->hdr, sizeof(reply->hdr));
		if (ret < 0) {
			if (errno != EAGAIN)
				return (-1);
		} else if (ret > 0) {
			break;
		}
	}

	while (1) {
		ret = read(fd, &reply->cmd, reply->hdr.length);
		if (ret < 0) {
			if (errno != EAGAIN)
				return (-1);
		} else if (ret > 0) {
			break;
		}
	}

	if (len > (sizeof(reply->hdr) + reply->hdr.length))
		len = sizeof(reply->hdr) + reply->hdr.length;

	memcpy(buf, tmp, len);

	return (len);
}
