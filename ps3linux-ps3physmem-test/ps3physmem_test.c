/*-
 * Copyright (C) 2014 glevand <geoffrey.levand@mail.ru>
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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define DEV_FILENAME	"/dev/ps3physmem"

static void hex_fprintf(FILE *fp, const unsigned char *buf, size_t len);

int
main(int argc, char **argv)
{
	int fd;
	unsigned char *buf;
	unsigned long addr, length;
	char *endptr;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <address> <length>\n", argv[0]);
		exit(1);
	}

	addr = strtoul(argv[1], &endptr, 0);
	if (*endptr != '\0') {
		fprintf(stderr, "error: invalid address: %s\n", argv[1]);
		exit(1);
	}

	length = strtoul(argv[2], &endptr, 0);
	if (*endptr != '\0') {
		fprintf(stderr, "error: invalid length: %s\n", argv[2]);
		exit(1);
	}

	fd = open(DEV_FILENAME, O_RDWR);
	if (fd < 0) {
		perror("open");
		exit(1);
	}

	buf = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, addr);
	if (buf == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}

	close(fd);

	hex_fprintf(stdout, buf, length);

	exit(0);
}

static void
hex_fprintf(FILE *fp, const unsigned char *buf, size_t len)
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
