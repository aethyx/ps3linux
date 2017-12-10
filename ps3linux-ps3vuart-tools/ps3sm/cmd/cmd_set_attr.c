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
#include <stdint.h>

#include <cmd.h>

#include "../opts.h"
#include "../dev.h"
#include "../ps3sm_msg.h"

int
cmd_set_attr_exec(int cmdc, struct cmd **cmdv, int argc, char **argv)
{
	struct ps3sm_set_attr set_attr;
	uint32_t attrs;
	char *endptr;
	int ret;

	attrs = strtoul(argv[0], &endptr, 0);
	if (*endptr != '\0')
		return (CMD_EINVAL);

	ps3sm_init_header(PS3SM_HDR(&set_attr),
	    sizeof(set_attr) - sizeof(struct ps3sm_header),
	    PS3SM_SID_SET_ATTR, 0);

	set_attr.version = PS3SM_SET_ATTR_VERSION;
	set_attr.attrs = attrs;

	ret = dev_write(&set_attr, sizeof(set_attr));
	if (ret != sizeof(set_attr))
		return (CMD_EIO);

	return (CMD_EOK);
}

struct cmd cmd_set_attr = {
	.name = "set_attr",
	.help = "set attribute(s)",
	.usage = "set_attr <attrs>",

	.min_argc = 1,
	.max_argc = 1,

	.exec = cmd_set_attr_exec,
};
