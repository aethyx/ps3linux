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
#include <util.h>

#include "../opts.h"
#include "../dev.h"
#include "../ps3sm_msg.h"

int
cmd_get_inter_lpar_param_exec(int cmdc, struct cmd **cmdv, int argc, char **argv)
{
	struct ps3sm_get_inter_lpar_param get_inter_lpar_param;
	struct ps3sm_get_inter_lpar_param_reply get_inter_lpar_param_reply;
	uint32_t arg;
	char *endptr;
	int ret;

	arg = strtoul(argv[0], &endptr, 0);
	if (*endptr != '\0' || arg < 1 || arg > 2)
		return (CMD_EINVAL);

	ps3sm_init_header(PS3SM_HDR(&get_inter_lpar_param),
	    sizeof(get_inter_lpar_param) - sizeof(struct ps3sm_header),
	    PS3SM_SID_GET_INTER_LPAR_PARAM, 0);

	get_inter_lpar_param.version = PS3SM_GET_INTER_LPAR_PARAM_VERSION;
	get_inter_lpar_param.arg = arg;

	ret = dev_write(&get_inter_lpar_param, sizeof(get_inter_lpar_param));
	if (ret != sizeof(get_inter_lpar_param))
		return (CMD_EIO);

	ret = dev_read(&get_inter_lpar_param_reply, sizeof(get_inter_lpar_param_reply));
	if (ret < 0)
		return (CMD_EIO);

	if (arg == 1)
		hex_fprintf(stdout, (char *) get_inter_lpar_param_reply.u.arg1.data,
		    sizeof(PS3SM_HDR(&get_inter_lpar_param_reply)->payload_length));
	else
		fprintf(stdout, "%02x %02x %02x %02x %08x %016llx\n",
		    get_inter_lpar_param_reply.u.arg2.val1,
		    get_inter_lpar_param_reply.u.arg2.val2,
		    get_inter_lpar_param_reply.u.arg2.val3,
		    get_inter_lpar_param_reply.u.arg2.val4,
		    get_inter_lpar_param_reply.u.arg2.val5,
		    get_inter_lpar_param_reply.u.arg2.val6);

	return (CMD_EOK);
}

struct cmd cmd_get_inter_lpar_param = {
	.name = "get_inter_lpar_param",
	.help = "get inter lpar parameter",
	.usage = "get_inter_lpar_param <arg>",

	.min_argc = 1,
	.max_argc = 1,

	.exec = cmd_get_inter_lpar_param_exec,
};
