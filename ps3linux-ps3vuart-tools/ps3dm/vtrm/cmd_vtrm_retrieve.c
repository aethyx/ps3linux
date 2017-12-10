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
cmd_vtrm_retrieve_exec(int cmdc, struct cmd **cmdv, int argc, char **argv)
{
	struct ps3dm_vtrm_retrieve retrieve;
	char *endptr;
	uint64_t nth;
	uint64_t laid, paid;
	int len;
	int ret;

	nth = strtoull(argv[0], &endptr, 0);
	if (*endptr != '\0')
		return (CMD_EINVAL);

	laid = opts.laid_valid ? opts.laid : 0x1070000002000001ull;
	paid = opts.paid_valid ? opts.paid : 0x10700003ff000001ull;

	if (opts.do_verbose > 0)
		fprintf(stderr, "laid %016llx paid %016llx\n", laid, paid);

	ps3dm_init_header(&retrieve.dm_hdr, 1, PS3DM_FID_VTRM,
	    sizeof(retrieve) - sizeof(struct ps3dm_header),
	    sizeof(retrieve) - sizeof(struct ps3dm_header));
	ps3dm_init_ss_header(&retrieve.ss_hdr, PS3DM_PID_VTRM_RETRIEVE,
	    laid, paid);
	retrieve.nth = nth;

	len = sizeof(retrieve);

	ret = dev_write(&retrieve, len);
	if (ret != len)
		return (CMD_EIO);

	ret = dev_read(&retrieve, sizeof(retrieve));
	if (ret < (sizeof(struct ps3dm_header) + sizeof(struct ps3dm_ss_header)))
		return (CMD_EIO);

	if (opts.do_verbose > 0)
		fprintf(stderr, "ss status %d\n", retrieve.ss_hdr.status);

	if (retrieve.ss_hdr.status)
		return (CMD_EIO);

	if (ret != sizeof(retrieve))
		return (CMD_EIO);

	fwrite(retrieve.data, 1, sizeof(retrieve.data), stdout);

	return (CMD_EOK);
}

struct cmd cmd_vtrm_retrieve = {
	.name = "retrieve",
	.help = "retrieve",
	.usage = "retrieve <nth>",

	.min_argc = 1,
	.max_argc = 1,

	.exec = cmd_vtrm_retrieve_exec,
};
