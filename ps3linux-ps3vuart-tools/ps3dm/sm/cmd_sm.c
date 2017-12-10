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

extern struct cmd cmd_sm_drive_auth;
extern struct cmd cmd_sm_get_rnd;
extern struct cmd cmd_sm_get_version;
extern struct cmd cmd_sm_set_del_encdec_key;
extern struct cmd cmd_sm_set_encdec_key;

static struct cmd *sm_cmdv[] = {
	&cmd_sm_drive_auth,
	&cmd_sm_get_rnd,
	&cmd_sm_get_version,
	&cmd_help,
	&cmd_list,
	&cmd_sm_set_del_encdec_key,
	&cmd_sm_set_encdec_key,
};

int
cmd_sm_exec(int cmdc, struct cmd **cmdv, int argc, char **argv)
{
	return (cmd_exec(sizeof(sm_cmdv) / sizeof(sm_cmdv[0]),
	    sm_cmdv, argc, argv));
}

struct cmd cmd_sm = {
	.name = "sm",
	.help = "storage manager",
	.usage = "sm <cmd> <arg>*",

	.min_argc = 1,
	.max_argc = -1,

	.exec = cmd_sm_exec,
};
