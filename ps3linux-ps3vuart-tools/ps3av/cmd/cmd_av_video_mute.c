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
cmd_av_video_mute_exec(int cmdc, struct cmd **cmdv, int argc, char **argv)
{
	struct ps3av_av_video_mute av_video_mute_req;
	struct ps3av_reply reply;
	char *endptr;
	unsigned int port, mute;
	int ret;

	port = strtoul(argv[0], &endptr, 0);
	if (*endptr != '\0')
		return (CMD_EINVAL);

	mute = strtoul(argv[1], &endptr, 0);
	if (*endptr != '\0')
		return (CMD_EINVAL);

	ps3av_init_request(PS3AV_REQ(&av_video_mute_req),
	    sizeof(av_video_mute_req) - sizeof(struct ps3av_header),
	    PS3AV_CMD_AV_VIDEO_MUTE);

	av_video_mute_req.port = port;
	av_video_mute_req.mute = mute;

	ret = dev_write(&av_video_mute_req, sizeof(av_video_mute_req));
	if (ret != sizeof(av_video_mute_req))
		return (CMD_EIO);

	ret = dev_read(&reply, sizeof(reply));
	if (ret != sizeof(reply))
		return (CMD_EIO);

	if (reply.status != PS3AV_STATUS_SUCCESS)
		return (CMD_EIO);

	return (CMD_EOK);
}

struct cmd cmd_av_video_mute = {
	.name = "av_video_mute",
	.help = "av video mute",
	.usage = "av_video_mute <port> <mute>",

	.min_argc = 2,
	.max_argc = 2,

	.exec = cmd_av_video_mute_exec,
};
