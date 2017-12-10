
/*
 * Copyright (C) 2011, 2012 glevand <geoffrey.levand@mail.ru>
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <ctype.h>

#include "util.h"

/*
 * hexdump
 */
void hexdump(const unsigned char *data, unsigned int data_size)
{
	int i, j;

	for (i = 0; i < data_size; i += 16) {
		fprintf(stdout, "%08x:", i);

		for (j = 0; j < 16; j++) {
			if (i + j < data_size) {
				fprintf(stdout, " %02x", data[i + j]);
			} else {
				fprintf(stdout, "   ");
			}
		}

		fprintf(stdout, " |");

		for (j = 0; j < 16; j++) {
			if (i + j < data_size) {
				if (isprint(data[i + j]))
					fprintf(stdout, "%c", data[i + j]);
				else
					fprintf(stdout, ".");
			} else {
				fprintf(stdout, " ");
			}
		}

		fprintf(stdout, "|\n");
	}
}
