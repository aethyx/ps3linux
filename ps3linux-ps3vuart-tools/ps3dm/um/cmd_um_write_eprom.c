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
#include "../ps3dm_msg.h"

int
cmd_um_write_eprom_exec(int cmdc, struct cmd **cmdv, int argc, char **argv)
{
	struct ps3dm_um_write_eprom write_eprom;
	uint32_t offset, data;
	char *endptr;
	uint64_t laid, paid;
	int len;
	int ret;

	offset = strtoul(argv[0], &endptr, 0);
	if (*endptr != '\0')
		return (CMD_EINVAL);

	data = strtoul(argv[1], &endptr, 0);
	if (*endptr != '\0' || data > 0xff)
		return (CMD_EINVAL);

	laid = opts.laid_valid ? opts.laid : 0x1070000002000001ull;
	paid = opts.paid_valid ? opts.paid : 0x10700003ff000001ull;

	ps3dm_init_header(&write_eprom.dm_hdr, 1, PS3DM_FID_UM,
	    sizeof(write_eprom) - sizeof(struct ps3dm_header),
	    sizeof(write_eprom) - sizeof(struct ps3dm_header));
	ps3dm_init_ss_header(&write_eprom.ss_hdr, PS3DM_PID_UM_WRITE_EPROM,
	    laid, paid);
	write_eprom.offset = offset;
	write_eprom.data = data;

	len = sizeof(write_eprom);

	ret = dev_write(&write_eprom, len);
	if (ret != len)
		return (CMD_EIO);

	ret = dev_write(&write_eprom, sizeof(write_eprom));
	if (ret != sizeof(write_eprom))
		return (CMD_EIO);

	if (opts.do_verbose > 0)
		fprintf(stderr, "ss status %d\n", write_eprom.ss_hdr.status);

	if (write_eprom.ss_hdr.status)
		return (CMD_EIO);

	return (CMD_EOK);
}

struct cmd cmd_um_write_eprom = {
	.name = "write_eprom",
	.help = "write eprom",
	.usage = "write_eprom <offset> <data>",

	.min_argc = 2,
	.max_argc = 2,

	.exec = cmd_um_write_eprom_exec,
};
