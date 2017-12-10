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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>

#include <cmd.h>

#include "opts.h"
#include "dev.h"

static struct option long_opts[] = {
	{ "help",	no_argument, NULL, 'h' },
	{ "version",	no_argument, NULL, 'V' },
	{ "verbose",	no_argument, NULL, 'v' },
	{ "device",	no_argument, NULL, 'd' },
	{ "laid",	no_argument, NULL, 'l' },
	{ "paid",	no_argument, NULL, 'p' },
	{ NULL, 0, NULL, 0 }
};

struct opts opts = {
	.device = "/dev/ps3dispmngr",
	.laid_valid = 0,
	.paid_valid = 0,
};

extern struct cmd cmd_aim;
extern struct cmd cmd_iim;
extern struct cmd cmd_scm;
extern struct cmd cmd_sm;
extern struct cmd cmd_srtc;
extern struct cmd cmd_um;
extern struct cmd cmd_vtrm;

static struct cmd *cmdv[] = {
	&cmd_aim,
	&cmd_help,
	&cmd_iim,
	&cmd_list,
	&cmd_scm,
	&cmd_sm,
	&cmd_srtc,
	&cmd_um,
	&cmd_vtrm,
};

static void
usage(void)
{
	exit(0);
}

static void
version(void)
{
	exit(0);
}

static int
parse_opts(int argc, char **argv, struct opts *opts)
{
	int c;
	char *endptr;

	while ((c = getopt_long(argc, argv, "hVvd:l:p:", long_opts, NULL)) != -1) {
		switch (c) {
		case 'h':
		case '?':
			opts->do_help = 1;
			return (0);
		break;
		case 'V':
			opts->do_version = 1;
			return (0);
		break;
		case 'v':
			opts->do_verbose++;
		break;
		case 'd':
			opts->device = optarg;
		break;
		case 'l':
			opts->laid = strtoull(optarg, &endptr, 0);
			if (*endptr != '\0') {
				fprintf(stderr, "invalid LAID specified\n");
				return (-1);
			}

			opts->laid_valid = 1;
		break;
		case 'p':
			opts->paid = strtoull(optarg, &endptr, 0);
			if (*endptr != '\0') {
				fprintf(stderr, "invalid PAID specified\n");
				return (-1);
			}

			opts->paid_valid = 1;
		break;
		default:
			fprintf(stderr, "invalid option specified: %c\n", c);
			return (-1);
		break;
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "no command specified\n");
		return (-1);
	}

	return (0);
}

int
main(int argc, char **argv)
{
	int ret;

	ret = parse_opts(argc, argv, &opts);
	if (ret)
		exit(255);

	if (opts.do_help)
		usage();
	else if (opts.do_version)
		version();

	ret = dev_open(opts.device);
	if (ret) {
		perror("open device");
		exit(254);
	}

	ret = cmd_exec(sizeof(cmdv) / sizeof(cmdv[0]), cmdv,
	    argc - optind, &argv[optind]);
	switch (ret) {
	case CMD_EOK:
	break;
	case CMD_EBADCMD:
		fprintf(stderr, "invalid command specified\n");
	break;
	case CMD_EINVAL:
		fprintf(stderr, "invalid command argument(s) specified\n");
	break;
	case CMD_EIO:
		fprintf(stderr, "device error\n");
	break;
	case CMD_EUNKNOWN:
		fprintf(stderr, "unknown command error\n");
	break;
	}

	dev_close();

	exit(ret);
}
