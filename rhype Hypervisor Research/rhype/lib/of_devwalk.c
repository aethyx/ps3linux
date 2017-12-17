/*
 * Copyright (C) 2005 Jimi Xenidis <jimix@watson.ibm.com>, IBM Corporation
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

#include <lib.h>
#include <openfirmware.h>
#include <of_devtree.h>

void
ofd_prop_print(const char *head, const char *path,
	       const char *name, const char *prop, uval sz)
{
	if (path[0] == '/' && path[1] == '\0') {
		path = "";
	}
	hprintf("%s: %s/%s: 0x%lx\n", head, path,  name, sz);

#define DEBUG_PROP
#ifdef DEBUG_PROP
	{
		uval i;
		int isstr = sz;
		const char *b = prop;

		for (i = 0; i < sz; i++) {
			/* see if there is any non printable characters */
			if (b[i] < ' ' || b[i] > '~') {
				/* not printable */
				if (b[i] != '\0' || (i + 1) != sz) {
					/* not the end of string */
					isstr = 0;
					break;
				}
			}
		}

		if (isstr) {
			hprintf("%s: \t%s\n", head, b);
		} else if (sz != 0) {
			hprintf("%s: \t0x", head);
			for (i = 0; i < sz; i++) {
				if ((i % 4) == 0 && i != 0) {
					if ((i % 16) == 0 && i != 0) {
						hprintf("\n%s: \t0x", head);
					} else {
						hputs(" 0x");
					}
				}
				if (b[i] < 0x10) {
					hputs("0");
				}
				hprintf("%x", b[i]);
			}
			hputs("\n");
		}
	}
#else
	prop = prop;
#endif

}


void
ofd_dump_props(void *mem, ofdn_t n, uval dump)
{
	ofdn_t p;
	char name[128];
	char prop[256] __attribute__ ((aligned (__alignof__ (uval64))));
	sval sz;
	const char *path;

	if (n == OFD_ROOT) {
		path = "";
	} else {
		path = ofd_node_path(mem, n);
	}

	if (dump & OFD_DUMP_NAMES) {
		hprintf("of_walk: %s: phandle 0x%x\n", path, n);
	}

	p = ofd_nextprop(mem, n, NULL, name);
	while (p > 0) {
		sz = ofd_getprop(mem, n, name, prop, sizeof (prop));
		if (sz > 0) {
			sz = MIN(sz, 256);
		}
		if (dump & OFD_DUMP_VALUES) {
			ofd_prop_print("of_walk", path, name, prop, sz);
		}
		p = ofd_nextprop(mem, n, name, name);
	}
}


void
ofd_walk(void *m, ofdn_t p, walk_fn fn, uval arg)
{
	ofdn_t n;

	(*fn)(m, p, arg);

	/* child */
	n = ofd_node_child(m, p);
	if (n != 0) {
		ofd_walk(m, n, fn, arg);
	}

	/* peer */
	n = ofd_node_peer(m, p);
	if (n != 0) {
		ofd_walk(m, n, fn, arg);
	}
}

