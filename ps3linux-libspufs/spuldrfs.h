/*
 * Copyright (C) 2012 glevand <geoffrey.levand@mail.ru>
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

#ifndef _SPULDRFS_H
#define _SPULDRFS_H

#define SPULDRFS_METLDR_NAME	"metldr"
#define SPULDRFS_LDR_NAME	"ldr"
#define SPULDRFS_BUF1_NAME	"buf1"
#define SPULDRFS_BUF2_NAME	"buf2"
#define SPULDRFS_BUF3_NAME	"buf3"
#define SPULDRFS_INFO_NAME	"info"
#define SPULDRFS_RUN_NAME	"run"
#define SPULDRFS_SHADOW_NAME	"shadow"
#define SPULDRFS_PRIV2_NAME	"priv2"
#define SPULDRFS_PROBLEM_NAME	"problem"

const char *spuldrfs_find(void);

int spuldrfs_run(const char *path);

#endif
