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

#ifndef _SPUFS_H
#define _SPUFS_H

const char *spufs_make_path(const char *dir, const char *file);

uint64_t spufs_get_kern_virt_addr(const char *path, const char *name);

int spufs_send_file(const char *src_path, const char *dst_path);

char *spufs_mmap_file(const char *path, size_t len, int flags);

int spufs_file_length(const char *path);

void spufs_hex_fprintf(FILE *fp, const unsigned char *buf, size_t len);

#endif
