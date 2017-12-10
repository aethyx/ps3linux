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

#include <stdlib.h>
#include <stdint.h>

#include <cmd.h>

#include "../opts.h"
#include "../dev.h"
#include "../ps3sm_msg.h"

int
cmd_ring_buzzer_exec(int cmdc, struct cmd **cmdv, int argc, char **argv)
{
	struct ps3sm_ring_buzzer ring_buzzer;
	uint32_t arg1, arg2, arg3;
	char *endptr;
	int ret;

	arg1 = strtoul(argv[0], &endptr, 0);
	if (*endptr != '\0' || arg1 > 0xff)
		return (CMD_EINVAL);

	arg2 = strtoul(argv[1], &endptr, 0);
	if (*endptr != '\0' || arg2 > 0xff)
		return (CMD_EINVAL);

	arg3 = strtoul(argv[2], &endptr, 0);
	if (*endptr != '\0')
		return (CMD_EINVAL);

	ps3sm_init_header(PS3SM_HDR(&ring_buzzer),
	    sizeof(ring_buzzer) - sizeof(struct ps3sm_header),
	    PS3SM_SID_RING_BUZZER, 0);

	ring_buzzer.version = PS3SM_RING_BUZZER_VERSION;
	ring_buzzer.arg1 = arg1;
	ring_buzzer.arg2 = arg2;
	ring_buzzer.arg3 = arg3;

	ret = dev_write(&ring_buzzer, sizeof(ring_buzzer));
	if (ret != sizeof(ring_buzzer))
		return (CMD_EIO);

	return (CMD_EOK);
}

struct cmd cmd_ring_buzzer = {
	.name = "ring_buzzer",
	.help = "ring buzzer",
	.usage = "ring_buzzer <arg1> <arg2> <arg3>",

	.min_argc = 3,
	.max_argc = 3,

	.exec = cmd_ring_buzzer_exec,
};
