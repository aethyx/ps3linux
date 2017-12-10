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

#include <asm/ps3strgmngr.h>

#include "../dev.h"
#include "cmd.h"

static struct {
	const char *str;
	uint64_t access_rights;
} access_rights_map[] = {
	{ "none", 0 },
	{ "r", PS3STRGMNGR_REGION_READ },
	{ "w", PS3STRGMNGR_REGION_WRITE },
	{ "rw", PS3STRGMNGR_REGION_READ | PS3STRGMNGR_REGION_WRITE },
};

int
cmd_set_acl_entry_exec(int cmdc, struct cmd **cmdv, int argc, char **argv)
{
#define N(a)	(sizeof(a) / sizeof((a)[0]))

	uint64_t devid, rgnid, laid;
	char *endptr;
	uint64_t access_rights = 0;
	int i;
	int ret;

	devid = strtoull(argv[0], &endptr, 0);
	if (*endptr != '\0')
		return (CMD_EINVAL);

	rgnid = strtoull(argv[1], &endptr, 0);
	if (*endptr != '\0')
		return (CMD_EINVAL);

	laid = strtoull(argv[2], &endptr, 0);
	if (*endptr != '\0')
		return (CMD_EINVAL);

	for (i = 0; i < N(access_rights_map); i++) {
		if (!strcmp(argv[3], access_rights_map[i].str)) {
			access_rights = access_rights_map[i].access_rights;
			break;
		}
	}

	if (i >= N(access_rights_map))
		return (CMD_EINVAL);

	ret = dev_set_region_acl_entry(devid, rgnid, laid, access_rights);
	if (ret)
		return (CMD_EIO);

	return (CMD_EOK);

#undef N
}

struct cmd cmd_set_acl_entry = {
	.name = "set_acl_entry",
	.help = "set region acl entry",
	.usage = "set_acl_entry <devid> <rgnid> <laid> <access rights>",

	.min_argc = 4,
	.max_argc = 4,

	.exec = cmd_set_acl_entry_exec,
};
