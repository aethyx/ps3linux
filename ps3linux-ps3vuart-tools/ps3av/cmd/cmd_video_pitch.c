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
cmd_video_pitch_exec(int cmdc, struct cmd **cmdv, int argc, char **argv)
{
	struct ps3av_video_pitch video_pitch_req;
	struct ps3av_reply reply;
	char *endptr;
	unsigned int head, pitch;
	int ret;

	head = strtoul(argv[0], &endptr, 0);
	if (*endptr != '\0')
		return (CMD_EINVAL);

	pitch = strtoul(argv[1], &endptr, 0);
	if (*endptr != '\0')
		return (CMD_EINVAL);

	ps3av_init_request(PS3AV_REQ(&video_pitch_req),
	    sizeof(video_pitch_req) - sizeof(struct ps3av_header),
	    PS3AV_CMD_VIDEO_PITCH);

	video_pitch_req.head = head;
	video_pitch_req.pitch = pitch;

	ret = dev_write(&video_pitch_req, sizeof(video_pitch_req));
	if (ret != sizeof(video_pitch_req))
		return (CMD_EIO);

	ret = dev_read(&reply, sizeof(reply));
	if (ret != sizeof(reply))
		return (CMD_EIO);

	if (reply.status != PS3AV_STATUS_SUCCESS)
		return (CMD_EIO);

	return (CMD_EOK);
}

struct cmd cmd_video_pitch = {
	.name = "video_pitch",
	.help = "video pitch",
	.usage = "video_pitch <head> <pitch>",

	.min_argc = 2,
	.max_argc = 2,

	.exec = cmd_video_pitch_exec,
};
