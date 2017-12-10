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
#include <string.h>
#include <stdint.h>

#include "../dev.h"
#include "cmd.h"

int
cmd_create_region_exec(int cmdc, struct cmd **cmdv, int argc, char **argv)
{
	uint64_t devid, start_block, nr_blocks, laid, rgnid;
	char *endptr;
	int ret;

	devid = strtoull(argv[0], &endptr, 0);
	if (*endptr != '\0')
		return (CMD_EINVAL);

	start_block = strtoull(argv[1], &endptr, 0);
	if (*endptr != '\0')
		return (CMD_EINVAL);

	nr_blocks = strtoull(argv[2], &endptr, 0);
	if (*endptr != '\0')
		return (CMD_EINVAL);

	laid = strtoull(argv[3], &endptr, 0);
	if (*endptr != '\0')
		return (CMD_EINVAL);

	ret = dev_create_region(devid, start_block, nr_blocks, laid, &rgnid);
	if (ret)
		return (CMD_EIO);

	fprintf(stdout, "%llu\n", rgnid);

	return (CMD_EOK);
}

struct cmd cmd_create_region = {
	.name = "create_region",
	.help = "create region",
	.usage = "create_region <devid> <start> <blocks> <laid>",

	.min_argc = 4,
	.max_argc = 4,

	.exec = cmd_create_region_exec,
};
