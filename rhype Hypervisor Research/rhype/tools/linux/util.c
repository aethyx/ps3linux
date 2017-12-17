/*
 * Copyright (C) 2005 Michal Ostrowski <mostrows@watson.ibm.com>, IBM Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id$
 *
 * Utility functions.
 *
 */


#include <hype_types.h>
#include <hype.h>
#include <hype_util.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <sys/ioctl.h>

const char* oh_pname = NULL;
static int hcall_fd = -1;

int
hcall_init(void) {
	if (hcall_fd == -1) {
		hcall_fd = open("/dev/hcall", O_RDWR);
	}
	if (hcall_fd < 0) {
		perror("opening hcall device");
		return -1;
	}

	return hcall_fd;
}

int
hcall(oh_hcall_args* hargs)
{
	if (hcall_fd == -1) {
		return -1;
	}
	return ioctl(hcall_fd, OH_GENHCALL, hargs);
}

int
sig_xirr_bind(uval32 xirr, int sig)
{
	oh_reflect_args ora;
	ora.oh_interrupt = xirr;
	ora.oh_signal = sig;
	return ioctl(hcall_fd, OH_IRQ_REFLECT, &ora);
}

uval
mem_hold(uval size)
{
	oh_mem_hold_args omha;
	omha.size = size;

	int ret = ioctl(hcall_fd, OH_MEM_HOLD, &omha);
	if (ret < 0) {
		perror("ioctl OH_MEM_HOLD:");
		return 0;
	}

	return omha.laddr;
}


int
parse_laddr(const char* arg, uval *base, uval *size)
{
	char *y;
	uval64 b = strtoull(arg, &y, 0);
	if (errno == ERANGE) return -1;

	if (strncmp(y, "MB", 2) == 0) {
		b <<= 20;
		y+=2;
	} else if (strncmp(y, "GB", 2) == 0) {
		b <<= 30;
		y+=2;
	}

	if (y[0] != ':') {
		return -1;
	}

	++y;

	uval64 s  = strtoull(y, &y, 0);
	if (errno == ERANGE) return -1;

	if (strncmp(y, "MB", 2) == 0) {
		s <<= 20;
		y+=2;
	} else if (strncmp(y, "GB", 2) == 0) {
		s <<= 30;
		y+=2;
	}

	if (base) *base = b;
	if (size) *size = s;

	return 0;
}


int
get_file(const char* name, char* buf, int len)
{
	char n[512];
	snprintf(n, 512, HYPE_ROOT "/%s/%s", oh_pname, name);

	int fd = open(n, O_RDONLY);
	if (fd < 0) {
		return 0;
	}

	int ret = read(fd, buf, len);
	close(fd);

	if (ret < 0) ret = 0;
	return ret;
}

int
get_file_numeric(const char* name, uval *val)
{
	char buf[65];
	int ret = get_file(name, buf, sizeof(buf)-1);
	if (ret <= 0) return -1;

	buf[ret] = 0;
	*val = strtoull(buf, NULL, 0);
	if (errno == ERANGE) return -1;
	return 0;
}

int
set_file(const char* name, const char* buf, int len)
{
	char n[512];
	snprintf(n, 512, HYPE_ROOT "/%s/%s", oh_pname, name);

	int fd = open(n, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	if (fd < 0) {
		return 0;
	}

	if (len == -1) {
		len = strlen((const char*)buf);
	}
	int ret = write(fd, buf, len);
	close(fd);

	if (ret < 0) ret = 0;
	return ret;
}

int
set_file_printf(const char* name, const char* fmt, ...)
{
	char buf[512];
	int len;
	va_list ap;
	va_start(ap, fmt);
	len = vsnprintf(buf, 512, fmt, ap);
	va_end(ap);
	return set_file(name, buf, len);
}

int
of_make_node(const char* name)
{
	char n[512];
	snprintf(n, 512, HYPE_ROOT "/%s/of_tree/%s", oh_pname, name);
	struct stat buf;
	int ret = stat(n, &buf);
	if (ret == 0 && !S_ISDIR(buf.st_mode)) {
		return -1;
	} else if (ret != 0) {
		ret = mkdir(n, 0755);
	}
	return ret;
}



int
of_set_prop(const char* node, const char* prop, const void* data, int len)
{
	char n[512];
	snprintf(n, 512, "of_tree/%s/%s", node, prop);
	return set_file(n, data, len);
}

void
oh_set_pname(const char* p)
{
	if (p == NULL) {
		printf("LPAR container name required (-n)\n");
		exit(-1);
	}
	oh_pname = p;

	char n[512];
	snprintf(n, 512, HYPE_ROOT "/%s", oh_pname);

	struct stat buf;
	int ret = stat(n, &buf);
	if (ret == 0 && !S_ISDIR(buf.st_mode)) {
		printf("'%s' is not a valid LPAR container\n",oh_pname);
		exit(-1);
	}
}
