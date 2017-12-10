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

#include <asm/ps3strgmngr.h>

#include "../dev.h"
#include "cmd.h"

static const char *dev_type_str[] = {
	"disk",
	"cdrom",
	"flash",
	"nor_flash"
};

int
cmd_print_device_exec(int cmdc, struct cmd **cmdv, int argc, char **argv)
{
#define N(a)	(sizeof(a) / sizeof((a)[0]))

	struct ps3strgmngr_device dev[PS3STRGMNGR_MAX_DEVICES];
	int nr_devices;
	char *endptr;
	uint64_t id = 0;
	int i;

	if (argc) {
		id = strtoull(argv[0], &endptr, 0);
		if (*endptr != '\0')
			return (CMD_EINVAL);
	}

	nr_devices = dev_get_devices(dev, N(dev));
	if (nr_devices < 0)
		return (CMD_EIO);

	for (i = 0; i < nr_devices; i++) {
		if (argc && (id != dev[i].id))
			continue;

		fprintf(stdout, "%10s %4llu %8llu %16llu %8llu\n",
		    dev_type_str[dev[i].type],
		    dev[i].id, dev[i].block_size, dev[i].nr_blocks,
		    dev[i].nr_regions);
	}

	return (CMD_EOK);

#undef N
}

struct cmd cmd_print_device = {
	.name = "print_device",
	.help = "print device(s)",
	.usage = "print_device [<id>]",

	.min_argc = 0,
	.max_argc = 1,

	.exec = cmd_print_device_exec,
};
