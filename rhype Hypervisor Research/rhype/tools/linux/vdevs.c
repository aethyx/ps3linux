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
#include <x86/os_args.h>

int root_len;

struct ofnode {
	const char* name;
	int len;
	ofdn_t parent;
	struct dirent *de;
};


struct description {
	const char * pattern;
	const char * name;
};

static const struct description desc[] = {
	{"/vdevice/vty@"   ,"ibmvty"},
	{"/vdevice/l-lan@" ,"ibmveth"},
	{"/vdevice/v-scsi@","ibmvscsi"},
	{"/vdevice/v-tpm@" ,"ibmvtpm"},
	{NULL, NULL}
};

static void
desc_append(char *descs, uval descs_len, const char *name,
	    uval32 liobn, uval32 dma_sz)
{
	char buf[256];
	snprintf(buf, sizeof(buf), "%s=0x%x,0x%x,0x%x,0x%x ",
	         name, liobn, liobn, dma_sz, liobn);
	strncat(descs, buf, descs_len);
}

static void 
process_dir(struct ofnode *on, char *descs, uval descs_len)
{
	ofdn_t node = on->parent;
	const char *ofname = on->name;
	int i = 0;
	int found = 0;

	DIR* dir = opendir(on->name);
	if (dir == NULL) {
		printf("can't open directory %s\n", on->name);
		exit(-1);
	}

	while (desc[i].pattern != NULL) {
		char *start = strstr(ofname, desc[i].pattern);
		if (NULL != start) {
			char filename[256];
			struct stat sbuf;
			snprintf(filename, sizeof(filename), 
			         "%s/ibm,my-dma-window",ofname);
			if (0 == stat(filename, &sbuf)) {
				uval32 dma[5];
				found = 1;
				if (sbuf.st_size == sizeof(dma)) {
					int fd = open(filename, O_RDONLY);
					if (fd > 0) {
						uval32 liobn, dma_sz;
						read(fd, &dma, sizeof(dma));
						liobn = dma[0];
						dma_sz = dma[4];
						desc_append(descs,
						            descs_len,
						            desc[i].name,
						            liobn,
						            dma_sz);
						close(fd);
					}
				}
			}
		}
		i++;
	}

	if (found == 0) {
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
				process_dir(&new, descs, descs_len);
			}
		}
	}
	closedir(dir);
}


const int def_size = 1 << 19;

static void
usage()
{
	printf("# vdevs usage:\n"
	       "#\tvdevs pack <dir> <output>\n"
	       "#\t     Pack up vio devs found in \"dir\" into \"output\"\n");
}

int main(int argc, char **argv)
{
	++argv; --argc;
	if (strcmp(*argv,"pack") == 0) {
		struct os_args args;
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
		process_dir(&n,
			    args.vdevargs,
			    sizeof(args.vdevargs));
		write(fd, args.vdevargs, strlen(args.vdevargs)+1);
		close(fd);
	} else {
		usage();
		return -1;
	}
	return 0;
}
