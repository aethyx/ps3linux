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

#ifndef _SPUISOFS_H
#define _SPUISOFS_H

#define SPUISOFS_APP_NAME	"app"
#define SPUISOFS_ARG1_NAME	"arg1"
#define SPUISOFS_ARG2_NAME	"arg2"
#define SPUISOFS_BUF_NAME	"buf"
#define SPUISOFS_INFO_NAME	"info"
#define SPUISOFS_RUN_NAME	"run"
#define SPUISOFS_CONT_NAME	"cont"
#define SPUISOFS_SHADOW_NAME	"shadow"
#define SPUISOFS_PRIV2_NAME	"priv2"
#define SPUISOFS_PROBLEM_NAME	"problem"

const char *spuisofs_find(void);

int spuisofs_run(const char *path, uint64_t laid, uint64_t arg1_size, uint64_t arg2_size);

int spuisofs_cont(const char *path);

#endif
