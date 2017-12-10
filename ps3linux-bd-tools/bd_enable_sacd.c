/*-
 * Copyright (C) 2013 glevand <geoffrey.levand@mail.ru>
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
#include <string.h>
#include <getopt.h>

#include "bd_util.h"

static struct option long_opts[] = {
	{ "device",	no_argument, NULL, 'd' },
	{ "mode",	no_argument, NULL, 'm' },
	{ NULL, 0, NULL, 0 }
};

static const char *device = "/dev/sr0";
static int mode = -1;

static int
parse_opts(int argc, char **argv)
{
	int c;
	char *endptr;

	while ((c = getopt_long(argc, argv, "d:m:", long_opts, NULL)) != -1) {
		switch (c) {
		case 'd':
			device = optarg;
		break;
		case 'm':
			mode = strtol(optarg, &endptr, 0);
			if (*endptr != '\0' || (mode != 0 && mode != 1)) {
				fprintf(stderr, "invalid mode specified\n");
				return (-1);
			}
		break;
		default:
			fprintf(stderr, "invalid option specified: %c\n", c);
			return (-1);
		break;
		}
	}

	return (0);
}

int
main(int argc, char **argv)
{
	unsigned char flag;
	unsigned int req_sense;
	int ret;

	ret = parse_opts(argc, argv);
	if (ret)
		exit(255);

	if (mode != -1) {
		flag = mode ? 0x53 : 0xff;
	
		fprintf(stdout, "setting flag to %x\n", flag);

		ret = sacd_d7_set(device, flag, &req_sense);
		if (ret) {
			fprintf(stderr, "SACD D7 SET failed: req sense %x\n", req_sense);
			exit(1);
		}
	}

	ret = sacd_d7_get(device, &flag, &req_sense);
	if (ret) {
		fprintf(stderr, "SACD D7 GET failed: req sense %x\n", req_sense);
		exit(1);
	}

	fprintf(stdout, "current flag %x\n", flag);

	exit(0);
}
