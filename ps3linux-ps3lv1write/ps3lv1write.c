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
#include <stdint.h>
#include <getopt.h>
#include <errno.h>

#include "dev.h"

#define CONS_ID		1
#define CONS_LENGTH	0xff0

static struct option long_opts[] = {
	{ "device", no_argument, NULL, 'd' },
	{ NULL, 0, NULL, 0 }
};

static const char *device = "/dev/ps3lv1call";

/*
 * parse_opts
 */
static int parse_opts(int argc, char **argv)
{
	int c;

	while ((c = getopt_long(argc, argv, "d:", long_opts, NULL)) != -1) {
		switch (c) {
		case 'd':
			device = optarg;
		break;
		default:
			fprintf(stderr, "invalid option specified: %c\n", c);
			return (-1);
		break;
		}
	}

	return (0);
}

static void lv1_console_init(uint64_t id, uint64_t len)
{
	uint64_t in_args[8];
	int num_in_args;
	uint64_t out_args[8];
	int64_t result;
	int ret;

	in_args[0] = id;
	in_args[1] = 0;
	in_args[2] = 0;
	in_args[3] = len;
	in_args[4] = len;
	in_args[5] = 0;
	in_args[6] = 0;
	num_in_args = 7;

	ret = dev_do_lv1call(device, 105, in_args, num_in_args, out_args, &result);
	if (ret < 0)
		fprintf(stderr, "lv1call 105 failed\n");
}

static void lv1_console_putchar(uint64_t id, int c)
{
	uint64_t in_args[8];
	int num_in_args;
	uint64_t out_args[8];
	int64_t result;
	uint64_t data;
	int ret;

	if (c == '\n')
		c = '\r';

	data = c;
	data <<= 56;

	in_args[0] = id;
	in_args[1] = 1;
	in_args[2] = data;
	in_args[3] = 0;
	in_args[4] = 0;
	in_args[5] = 0;
	num_in_args = 6;

	ret = dev_do_lv1call(device, 107, in_args, num_in_args, out_args, &result);
	if (ret < 0)
		fprintf(stderr, "lv1call 107 failed\n");
}

static void lv1_console_flush(uint64_t id)
{
	uint64_t in_args[8];
	int num_in_args;
	uint64_t out_args[8];
	int64_t result;
	int ret;

	in_args[0] = id;
	num_in_args = 1;

	ret = dev_do_lv1call(device, 109, in_args, num_in_args, out_args, &result);
	if (ret < 0)
		fprintf(stderr, "lv1call 107 failed\n");
}

/*
 * main
 */
int main(int argc, char **argv)
{
	int i, j;
	int ret;

	ret = parse_opts(argc, argv);
	if (ret)
		return (255);

	lv1_console_init(CONS_ID, CONS_LENGTH);

	for (i = optind; i < argc; i++) {
		for (j = 0; j < strlen(argv[i]); j++)
			lv1_console_putchar(CONS_ID, argv[i][j]);
		lv1_console_putchar(CONS_ID, '\n');
		lv1_console_flush(CONS_ID);
	}

	return (0);
}
