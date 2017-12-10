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
#include <string.h>

#include <cmd.h>
#include <util.h>

#include "../opts.h"
#include "../dev.h"
#include "../ps3dm_msg.h"

int
cmd_sm_set_encdec_key_exec(int cmdc, struct cmd **cmdv, int argc, char **argv)
{
	struct ps3dm_sm_set_encdec_key set_encdec_key;
	uint64_t laid, paid;
	uint8_t key[24];
	int key_length;
	int ret;

	key_length = parse_hex(argv[0], key, sizeof(key));
	if (key_length <= 0)
		return (CMD_EINVAL);

	laid = opts.laid_valid ? opts.laid : 0x1070000002000001ull;
	paid = opts.paid_valid ? opts.paid : 0x10700003ff000001ull;

	ps3dm_init_header(&set_encdec_key.dm_hdr, 1, PS3DM_FID_SM,
	    sizeof(set_encdec_key) - sizeof(struct ps3dm_header),
	    sizeof(set_encdec_key) - sizeof(struct ps3dm_header));
	ps3dm_init_ss_header(&set_encdec_key.ss_hdr, PS3DM_PID_SM_SET_ENCDEC_KEY,
	    laid, paid);
	memset(set_encdec_key.key, 0, sizeof(set_encdec_key.key));
	memcpy(set_encdec_key.key, key, key_length);
	set_encdec_key.key_length = key_length;

	ret = dev_write(&set_encdec_key, sizeof(set_encdec_key));
	if (ret != sizeof(set_encdec_key))
		return (CMD_EIO);

	ret = dev_read(&set_encdec_key, sizeof(set_encdec_key));
	if (ret != sizeof(set_encdec_key))
		return (CMD_EIO);

	if (opts.do_verbose > 0)
		fprintf(stderr, "ss status %d\n", set_encdec_key.ss_hdr.status);

	if (set_encdec_key.ss_hdr.status)
		return (CMD_EIO);

	fprintf(stdout, "%llx\n", set_encdec_key.key_index);

	return (CMD_EOK);
}

struct cmd cmd_sm_set_encdec_key = {
	.name = "set_encdec_key",
	.help = "set encdec key",
	.usage = "set_encdec_key <key>",

	.min_argc = 1,
	.max_argc = 1,

	.exec = cmd_sm_set_encdec_key_exec,
};
