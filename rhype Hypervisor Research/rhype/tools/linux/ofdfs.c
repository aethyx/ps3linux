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
 */
#include <stdio.h>
#include <unistd.h>
#include <types.h>
#include <of_devtree.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

int root_len;
void *mem;

extern int hprintf(const char *fmt, ...);
extern void hputs(const char* str);
extern void assprint(const char *expr, const char *file, 
		     int line, const char *fmt, ...);


int 
hprintf(const char *fmt, ...)
{
	int ret;
	va_list ap;
	va_start(ap, fmt);
	ret = vprintf(fmt, ap);
	va_end(ap);
	return ret;
}

void
hputs(const char* str)
{
	printf(str);
}


void
assprint(const char *expr, const char *file, int line, const char *fmt, ...)
{
	va_list ap;

	/* I like compiler like output */
	printf("%s:%d: ASSERT(%s)\n", file, line, expr);

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

struct ofnode {
	const char* name;
	int len;
	ofdn_t parent;
	struct dirent *de;
};

static void 
process_file(struct ofnode *on)
{
	struct stat sbuf;
	int i = 0;
	int ret = stat(on->name, &sbuf);
	int fd = open(on->name, O_RDONLY);
	if (fd < 0) {
		printf("can't open file '%s' %d\n", on->name, errno);
		exit(-1);
	}
	char* buf = (char*)malloc(sbuf.st_size + 1);
	buf[sbuf.st_size] = 0;

	ret = read(fd, buf, sbuf.st_size);
	if (ret != sbuf.st_size) {
		printf("bad read: %s %d vs %ld errno: %d",
		       on->name, ret, sbuf.st_size, errno);
		exit(-1);
	}

	/* If all printable, add trailing nil char */
	while(i < ret){
		if(!isprint(buf[i])){
			break;
		}
		i++;
	}
	if(i == ret && ret > 0){
		++ret;
	}

	close(fd);
	ofd_prop_add(mem, on->parent, on->de->d_name, buf, ret);
	free(buf);
}

static void 
process_dir(struct ofnode *on)
{
	ofdn_t node = on->parent;
	const char *ofname = on->name;
	if (on->de) {
		int len = strlen(on->name) - root_len;
		node = ofd_node_add(mem, on->parent, on->name + root_len, len);
	}

	DIR* dir = opendir(on->name);
	if (dir == NULL) {
		printf("can't open directory %s\n", on->name);
		exit(-1);
	}

	struct dirent *de;
	while ((de = readdir(dir))) {
		if (strcmp(de->d_name,".") == 0 ||
		   strcmp(de->d_name,"..") == 0) {
			continue;
		}
		int nlen = on->len + strlen(de->d_name) + 2;
		char* str = (char*)malloc(nlen);
		snprintf(str, nlen, "%s/%s", ofname, de->d_name);
		struct ofnode new;
		new.de = de;
		new.name = str;
		new.len = nlen;
		new.parent = node;
		if (de->d_type == DT_DIR) {
			process_dir(&new);
		} else if (de->d_type == DT_REG) {
			process_file(&new);
		}
	}

	closedir(dir);

}


const int def_size = 1 << 19;

static void
usage()
{
	printf("# ofdfs usage:\n"
	       "#\tofdfs pack <dir> <output>\n"
	       "#\t     Pack up \"dir\" into \"output\"\n"
	       "#\tofdfs scan <file>\n"
	       "#\t      Print contents of OF data file\n");
}

int main(int argc, char **argv)
{
	void *buf = mmap(NULL,def_size, PROT_READ|PROT_WRITE,
			 MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	++argv; --argc;
	if (strcmp(*argv,"pack") == 0) {
		mem = ofd_create(buf, def_size);
		struct ofnode n;
		++argv; --argc;
		n.name = *argv;
		root_len = n.len = strlen(*argv);

		++argv; --argc;
		int fd = open(*argv, O_RDWR|O_CREAT, 0644);

		if (fd < 0) {
			printf("Can't open %s for writing: %s\n",
			       *argv, strerror(errno));
			exit(-1);
		}

		n.parent = OFD_ROOT;
		n.de = NULL;
		process_dir(&n);

		write(fd, mem, def_size);
	} else if (strcmp(*argv, "scan") == 0) {
		++argv; --argc;
		int fd = open(*argv, O_RDONLY);

		if (fd < 0) {
			printf("Can't open %s for reading: %s\n",
			       *argv, strerror(errno));
			exit(-1);
		}

		read(fd, buf, def_size);

		ofd_walk(buf, OFD_ROOT, ofd_dump_props, OFD_DUMP_ALL);
	} else {
		usage();
		return -1;
	}
	return 0;
}
