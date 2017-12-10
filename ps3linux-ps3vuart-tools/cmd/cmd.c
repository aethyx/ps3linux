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
#include <string.h>

#include "cmd.h"

struct cmd *
cmd_find(int cmdc, struct cmd **cmdv, const char *name)
{
	int l, r, m;
	int cmp;

	l = 0;
	r = cmdc - 1;

	while (l <= r) {
		m = (l + r) / 2;

		cmp = strcmp(name, cmdv[m]->name);
		if (!cmp)
			return (cmdv[m]);
		else if (cmp < 0)
			r = m - 1;
		else
			l = m + 1;

	}

	return (NULL);
}

int
cmd_exec(int cmdc, struct cmd **cmdv, int argc, char **argv)
{
	struct cmd *cmd;

	if (argc <= 0)
		return (CMD_EBADCMD);

	cmd = cmd_find(cmdc, cmdv, argv[0]);
	if (!cmd)
		return (CMD_EBADCMD);

	if ((cmd->min_argc != -1 && (argc - 1) < cmd->min_argc) ||
	    (cmd->max_argc != -1 && (argc - 1) > cmd->max_argc))
		return (CMD_EINVAL);

	return (cmd->exec(cmdc, cmdv, argc - 1, argv + 1));
}
