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
cmd_ctl_led_exec(int cmdc, struct cmd **cmdv, int argc, char **argv)
{
	struct ps3sm_ctl_led ctl_led;
	uint32_t arg1, arg2, arg3, arg4;
	char *endptr;
	int ret;

	arg1 = strtoul(argv[0], &endptr, 0);
	if (*endptr != '\0' || arg1 > 0xff)
		return (CMD_EINVAL);

	arg2 = strtoul(argv[1], &endptr, 0);
	if (*endptr != '\0' || arg2 > 0xff)
		return (CMD_EINVAL);

	arg3 = strtoul(argv[2], &endptr, 0);
	if (*endptr != '\0' || arg3 > 0xff)
		return (CMD_EINVAL);

	arg4 = strtoul(argv[3], &endptr, 0);
	if (*endptr != '\0' || arg4 > 0xff)
		return (CMD_EINVAL);

	ps3sm_init_header(PS3SM_HDR(&ctl_led),
	    sizeof(ctl_led) - sizeof(struct ps3sm_header),
	    PS3SM_SID_CTL_LED, 0);

	ctl_led.version = PS3SM_CTL_LED_VERSION;
	ctl_led.arg1 = arg1;
	ctl_led.arg2 = arg2;
	ctl_led.arg3 = arg3;
	ctl_led.arg4 = arg4;

	ret = dev_write(&ctl_led, sizeof(ctl_led));
	if (ret != sizeof(ctl_led))
		return (CMD_EIO);

	return (CMD_EOK);
}

struct cmd cmd_ctl_led = {
	.name = "ctl_led",
	.help = "control led",
	.usage = "ctl_led <arg1> <arg2> <arg3> <arg4>",

	.min_argc = 4,
	.max_argc = 4,

	.exec = cmd_ctl_led_exec,
};
