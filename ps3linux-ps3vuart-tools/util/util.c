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
#include <string.h>
#include <ctype.h>

#include "util.h"

int
parse_hex(const char *s, unsigned char *b, int maxlen)
{
	int len;
	char c;
	int i;

	len = strlen(s);
	if (len % 2)
		return (-1);

	for (i = 0; (i < (len / 2)) && (i < maxlen); i++) {
		if (!isxdigit(s[2 * i + 0]) || !isxdigit(s[2 * i + 1]))
			return (-1);

		b[i] = 0;

		c = tolower(s[2 * i + 0]);
		if (isdigit(c))
			b[i] += (c - '0') * 16;
		else
			b[i] += (c - 'a' + 10) * 16;

		c = tolower(s[2 * i + 1]);
		if (isdigit(c))
			b[i] += c - '0';
		else
			b[i] += c - 'a' + 10;
	}

	return (i);
}

void
hex_fprintf(FILE *fp, const char *buf, size_t len)
{
	int i;

	if (len <= 0)
		return;

	for (i = 0; i < len; i++) {
		if ((i > 0) && !(i % 16))
			fprintf(fp, "\n");

		fprintf(fp, "%02x ", buf[i]);
	}

	fprintf(fp, "\n");
}
