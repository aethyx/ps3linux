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

#include <cmd.h>

#include "../opts.h"
#include "../dev.h"
#include "../ps3av_msg.h"

int
cmd_av_get_hw_conf_exec(int cmdc, struct cmd **cmdv, int argc, char **argv)
{
	struct ps3av_request req;
	struct ps3av_av_get_hw_conf_reply av_get_hw_conf_reply;
	int ret;

	ps3av_init_request(&req, sizeof(req) - sizeof(struct ps3av_header),
	    PS3AV_CMD_AV_GET_HW_CONF);

	ret = dev_write(&req, sizeof(req));
	if (ret != sizeof(req))
		return (CMD_EIO);

	ret = dev_read(&av_get_hw_conf_reply, sizeof(av_get_hw_conf_reply));
	if (ret != sizeof(av_get_hw_conf_reply))
		return (CMD_EIO);

	if (av_get_hw_conf_reply.reply.status != PS3AV_STATUS_SUCCESS)
		return (CMD_EIO);

	fprintf(stdout, "%d %d %d\n", av_get_hw_conf_reply.nr_hdmi,
	    av_get_hw_conf_reply.nr_avmulti, av_get_hw_conf_reply.nr_spdif);

	return (CMD_EOK);
}

struct cmd cmd_av_get_hw_conf = {
	.name = "av_get_hw_conf",
	.help = "get av hardware configuration",
	.usage = "av_get_hw_conf",

	.min_argc = 0,
	.max_argc = 0,

	.exec = cmd_av_get_hw_conf_exec,
};
