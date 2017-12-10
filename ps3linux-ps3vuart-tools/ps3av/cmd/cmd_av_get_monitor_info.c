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
cmd_av_get_monitor_info_exec(int cmdc, struct cmd **cmdv, int argc, char **argv)
{
	struct ps3av_av_get_monitor_info av_get_monitor_info_req;
	struct ps3av_av_get_monitor_info_reply av_get_monitor_info_reply;
	char *endptr;
	unsigned int port;
	int ret;

	port = strtoul(argv[0], &endptr, 0);
	if (*endptr != '\0')
		return (CMD_EINVAL);

	ps3av_init_request(PS3AV_REQ(&av_get_monitor_info_req),
	    sizeof(av_get_monitor_info_req) - sizeof(struct ps3av_header),
	    PS3AV_CMD_AV_GET_MONITOR_INFO);

	av_get_monitor_info_req.port = port;

	ret = dev_write(&av_get_monitor_info_req, sizeof(av_get_monitor_info_req));
	if (ret != sizeof(av_get_monitor_info_req))
		return (CMD_EIO);

	ret = dev_read(&av_get_monitor_info_reply, sizeof(av_get_monitor_info_reply));
	if (ret < sizeof(struct ps3av_reply))
		return (CMD_EIO);

	if (opts.do_verbose)
		fprintf(stderr, "status %d\n", av_get_monitor_info_reply.reply.status);

	if (av_get_monitor_info_reply.reply.status != PS3AV_STATUS_SUCCESS)
		return (CMD_EIO);

	fprintf(stdout, "type: %d\n", av_get_monitor_info_reply.monitor.type);
	fprintf(stdout, "name: %s\n", av_get_monitor_info_reply.monitor.name);
	fprintf(stdout, "res_60: %08x %08x\n", av_get_monitor_info_reply.monitor.res_60.bits,
	    av_get_monitor_info_reply.monitor.res_60.native);
	fprintf(stdout, "res_50: %08x %08x\n", av_get_monitor_info_reply.monitor.res_50.bits,
	    av_get_monitor_info_reply.monitor.res_50.native);
	fprintf(stdout, "res_other: %08x %08x\n", av_get_monitor_info_reply.monitor.res_other.bits,
	    av_get_monitor_info_reply.monitor.res_other.native);
	fprintf(stdout, "res_vesa: %08x %08x\n", av_get_monitor_info_reply.monitor.res_vesa.bits,
	    av_get_monitor_info_reply.monitor.res_vesa.native);

	return (CMD_EOK);
}

struct cmd cmd_av_get_monitor_info = {
	.name = "av_get_monitor_info",
	.help = "get av monitor info",
	.usage = "av_get_monitor_info <port>",

	.min_argc = 1,
	.max_argc = 1,

	.exec = cmd_av_get_monitor_info_exec,
};
