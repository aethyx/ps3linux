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
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <errno.h>

#include "dev.h"

static struct option long_opts[] = {
	{ "device", no_argument, NULL, 'd' },
	{ NULL, 0, NULL, 0 }
};

static const char *device = "/dev/ps3lv1call";
static uint64_t num_lv1call;
static uint64_t in_args[8];
static int num_in_args;

/*
 * parse_opts
 */
static int parse_opts(int argc, char **argv)
{
	int c, i;
	char *endptr;

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

	if (optind >= argc) {
		fprintf(stderr, "no lv1 call number specified\n");
		return (-1);
	}

	num_lv1call = strtoull(argv[optind], &endptr, 0);
	if ((*endptr != '\0') || (num_lv1call > 255)) {
		fprintf(stderr, "invalid lv1 call number specified: %s\n", argv[optind]);
		return (-1);
	}

	optind++;

	num_in_args = argc - optind;
	if (num_in_args > 8) {
		fprintf(stderr, "too many lv1 call input arguments specified\n");
		return (-1);
	}

	for (i = 0; i < num_in_args; i++) {
		in_args[i] = strtoull(argv[optind + i], &endptr, 0);
		if (*endptr != '\0') {
			fprintf(stderr, "invalid lv1 call input argument specified: %s\n",
				argv[optind + i]);
			return (-1);
		}
	}

	return (0);
}

/*
 * main
 */
int main(int argc, char **argv)
{
	uint64_t out_args[7];
	int64_t result;
	int n, i, ret;

	ret = parse_opts(argc, argv);
	if (ret)
		return (255);

	n = dev_do_lv1call(device, num_lv1call, in_args, num_in_args, out_args, &result);
	if (n < 0) {
		fprintf(stderr, "device error %d\n", errno);
		return (1);
	}

	if (result) {
		fprintf(stderr, "lv1 call %llu error %lld\n", num_lv1call, result);
		return (2);
	}

	for (i = 0; i < n; i++)
		fprintf(stdout, "%016llx%s", out_args[i], i == (n - 1) ? "\n" : " ");

	return (0);
}
