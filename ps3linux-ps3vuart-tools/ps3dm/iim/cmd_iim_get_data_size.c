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
cmd_iim_get_data_size_exec(int cmdc, struct cmd **cmdv, int argc, char **argv)
{
	struct ps3dm_iim_get_data_size get_data_size;
	uint64_t index;
	char *endptr;
	uint64_t laid, paid;
	int len;
	int ret;

	index = strtoull(argv[0], &endptr, 0);
	if (*endptr != '\0')
		return (CMD_EINVAL);

	laid = opts.laid_valid ? opts.laid : 0x1070000002000001ull;
	paid = opts.paid_valid ? opts.paid : 0x1070000300000001ull;

	ps3dm_init_header(&get_data_size.dm_hdr, 1, PS3DM_FID_IIM,
	    sizeof(get_data_size) - sizeof(struct ps3dm_header),
	    sizeof(get_data_size) - sizeof(struct ps3dm_header));
	ps3dm_init_ss_header(&get_data_size.ss_hdr, PS3DM_PID_IIM_GET_DATA_SIZE,
	    laid, paid);
	get_data_size.index = index;

	len = sizeof(struct ps3dm_iim_get_data_size);

	ret = dev_write(&get_data_size, len);
	if (ret != len)
		return (CMD_EIO);

	ret = dev_read(&get_data_size, sizeof(get_data_size));
	if (ret != sizeof(get_data_size))
		return (CMD_EIO);

	if (opts.do_verbose > 0)
		fprintf(stderr, "ss status %d\n", get_data_size.ss_hdr.status);

	if (get_data_size.ss_hdr.status)
		return (CMD_EIO);

	fprintf(stdout, "%llx\n", get_data_size.size);

	return (CMD_EOK);
}

struct cmd cmd_iim_get_data_size = {
	.name = "get_data_size",
	.help = "get data size",
	.usage = "get_data_size <index>",

	.min_argc = 1,
	.max_argc = 1,

	.exec = cmd_iim_get_data_size_exec,
};
