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

#include <cmd.h>
#include <util.h>

#include "../opts.h"
#include "../dev.h"
#include "../ps3dm_msg.h"

int
cmd_vtrm_store_with_update_exec(int cmdc, struct cmd **cmdv, int argc, char **argv)
{
	struct ps3dm_vtrm_store_with_update store_with_update;
	uint64_t laid, paid;
	unsigned char data[64];
	int data_length;
	int len;
	int ret;

	data_length = parse_hex(argv[0], data, sizeof(data));
	if (data_length <= 0)
		return (CMD_EINVAL);

	laid = opts.laid_valid ? opts.laid : 0x1070000002000001ull;
	paid = opts.paid_valid ? opts.paid : 0x10700003ff000001ull;

	if (opts.do_verbose > 0)
		fprintf(stderr, "laid %016llx paid %016llx\n", laid, paid);

	ps3dm_init_header(&store_with_update.dm_hdr, 1, PS3DM_FID_VTRM,
	    sizeof(store_with_update) - sizeof(struct ps3dm_header),
	    sizeof(store_with_update) - sizeof(struct ps3dm_header));
	ps3dm_init_ss_header(&store_with_update.ss_hdr, PS3DM_PID_VTRM_STORE_WITH_UPDATE,
	    laid, paid);
	memset(store_with_update.data, 0, sizeof(store_with_update.data));
	memcpy(store_with_update.data, data, data_length);

	len = sizeof(store_with_update);

	ret = dev_write(&store_with_update, len);
	if (ret != len)
		return (CMD_EIO);

	ret = dev_read(&store_with_update, sizeof(store_with_update));
	if (ret < (sizeof(struct ps3dm_header) + sizeof(struct ps3dm_ss_header)))
		return (CMD_EIO);

	if (opts.do_verbose > 0)
		fprintf(stderr, "ss status %d\n", store_with_update.ss_hdr.status);

	if (store_with_update.ss_hdr.status)
		return (CMD_EIO);

	if (ret != sizeof(store_with_update))
		return (CMD_EIO);

	return (CMD_EOK);
}

struct cmd cmd_vtrm_store_with_update = {
	.name = "store_with_update",
	.help = "store with update",
	.usage = "store_with_update <data>",

	.min_argc = 1,
	.max_argc = 1,

	.exec = cmd_vtrm_store_with_update_exec,
};
