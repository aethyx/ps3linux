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
/*
 * Test of put_term_buffer hcall.
 *
 */

#include <test.h>
#include <lib.h>
#include <openfirmware.h>
#include <of_devtree.h>
#include <util.h>
#include <mmu.h>

/* run in phys mode */
int do_map_myself = 0;

const char *err = "OF handler reported failure: %s\n";

static uval
dump_props(phandle pkg)
{
	sval ret;
	sval32 sz;
	char name[128];
	char path[128];
	sval32 result = 1;

	ret = call_of("package-to-path", 3, 1, &sz,
		      pkg, path, sizeof (path));
	assert(ret == OF_SUCCESS, "OF package-to-path failed\n");
	assert(((uval)sz + 2) < sizeof (path), "buffer not big enough\n");
	path[sz] = 0;

	hprintf("oftest: %s: phandle 0x%x\n", path, pkg);

	/* get first */
	ret = call_of("nextprop", 3, 1, &result, pkg, 0, name);
	assert(ret == OF_SUCCESS, "nextprop");

	while (result > 0) {
		sval32 actual;
		char obj[512] __attribute__ ((aligned (__alignof__ (uval64))));

		sz = of_getproplen(pkg, name);
		if (sz >= 0) {
			if (sz > 0) {
				assert((uval)sz <= sizeof(obj),
				       "obj array not big enough for 0x%x\n",
				       sz);
				ret = call_of("getprop", 4, 1, &actual,
					      pkg, name, obj, sz);
				assert(ret == OF_SUCCESS, "getprop");
				assert(actual <= sz, "obj too small");
			}
			obj[sz] = 0;
			ofd_prop_print("oftest", path, name,
				       obj, sz);
		}
		ret = call_of("nextprop", 3, 1, &result, pkg, name, name);
		assert(ret == OF_SUCCESS, "nextprop");
	}
	return 1;
}

static void
root_props(void)
{
	phandle pkg;
	sval ret;

	hputs("testing for property handlers ...");
	/* make sure the property of calls are supported */
	ret = call_of("test", 1, 1, &pkg, "getprop");
	assert(ret == OF_SUCCESS, err, "test(getprop)");
	ret = call_of("test", 1, 1, &pkg, "getproplen");
	assert(ret == OF_SUCCESS, err, "test(getproplen)");
	ret = call_of("test", 1, 1, &pkg, "nextprop");
	assert(ret == OF_SUCCESS, err, "test(nextprop)");
	ret = call_of("test", 1, 1, &pkg, "peer");
	assert(ret == OF_SUCCESS, err, "test(peer)");
	hputs("done.\n");
	hputs("Dumping root properties\n");

	/*
	 * first.. lets interrogate the the root pkg
	 */
	ret = call_of("peer", 1, 1, &pkg, 0);
	assert(ret == OF_SUCCESS, err, "peer");

	dump_props(pkg);

}

static void
do_walk(phandle p, char *path, uval psz)
{
	phandle pnext;
	sval r;

	dump_props(p);

	/* do children first */
	r = call_of("child", 1, 1, &pnext, p);

	if (pnext != 0) {
		do_walk(pnext, path, psz);
	}

	/* do peer */
	r = call_of("peer", 1, 1, &pnext, p);
	assert(r == OF_SUCCESS, "OF peer failed\n");

	if (pnext != 0) {
		do_walk(pnext, path, psz);
	}
}

static void
tree_walk(void)
{
	phandle froot;
	phandle root;
	sval ret;
	char path[256];

	hputs("testing for devtree walk handlers ...");
	/* make sure the dev tree of calls are supported */
	ret = call_of("test", 1, 1, &root, "parent");
	assert(ret == OF_SUCCESS, err, "test(parent)");
	ret = call_of("test", 1, 1, &root, "child");
	assert(ret == OF_SUCCESS, err, "test(child)");
	ret = call_of("test", 1, 1, &root, "peer");
	assert(ret == OF_SUCCESS, err, "test(peer)");
	hputs("done.\n");

	hputs("Walking devtree\n");

	froot = of_finddevice("/");

	ret = call_of("peer", 1, 1, &root, 0);
	assert(ret == OF_SUCCESS, "OF peer for root failed\n");
	assert(froot == root,
	       "finddevice(\"/\") different form peer of zero\n");

	do_walk(root, path, sizeof (path));
}

static void
console(void)
{
	phandle pkg;
	ihandle of_stdin;
	ihandle of_stdout;
	char out[128];
	char in[128];
	sval32 res;
	uval sz;
	sval ret;

	pkg = of_finddevice("/chosen");
	hprintf("Finding /chosen: %x\n", pkg);

	hputs("Finding /chosen/stdin and /chosen/stdout\n");
	ret = of_getprop(pkg, "stdin", &of_stdin, sizeof (of_stdin));
	assert(ret != OF_FAILURE, "of_getprop(stdin)");

	ret = of_getprop(pkg, "stdout", &of_stdout, sizeof (of_stdout));
	assert(ret != OF_FAILURE, "of_getprop(stdout)");

	ret = call_of("instance-to-path", 3, 1, &res,
		      of_stdin, in, sizeof (in));
	assert(ret != OF_FAILURE, "instance-to-path(stdin)");
	hprintf("stdin [0x%x]: %s\n", of_stdin, in);

	ret = call_of("instance-to-path", 3, 1, &res,
		      of_stdout, out, sizeof (out));
	assert(ret != OF_FAILURE, "instance-to-path(stdout)");
	hprintf("stdout[0x%x]: %s\n", of_stdin, out);

	hputs("testing /chosen/stdout\n");

	sz = snprintf(out, sizeof(out),
		      "OpenFirmware wrote this: 0x%x\n\r", of_stdout);
	ret = of_write(of_stdout, out, sz);
	assert(ret != OF_FAILURE, "of_write(stdout)");

	sz = snprintf(out, sizeof(out),
		      "Type in something on 0x%x [Q to quit]:\n\r", of_stdin);
	of_write(of_stdout, out, sz);

	do {
		sz = of_read(of_stdin, in, sizeof(in));
		if (sz > 0) {
			sz = snprintf(out, sizeof(out),
				      "of_stdin: %s", in);
			of_write(of_stdout, out, sz);
			of_write(of_stdout, "\n\r", 2);
		}
	} while (in[0] != 'Q' && in[1] != '\n');
}

extern uval _end[];
static void
rtas(void)
{
	phandle pkg;
	ihandle irtas;
	sval32 res[2];
	uval32 loc = ALIGN_UP((uval)_end, PGSIZE);
	sval ret;

	hputs("Finding /rtas\n");
	pkg = of_finddevice("/rtas");
	assert(pkg > 0, " /rtas not found\n");

	hputs("opening /rtas\n");
	ret = call_of("open", 1, 1, &irtas, "/rtas");
	assert(ret == OF_SUCCESS, err, "open(/rtas)");

	hprintf("instantiate-rtas: location: 0x%x\n", loc);
	ret = call_of("call-method", 3, 2, res,
		      "instantiate-rtas", irtas, loc);
	assert(ret == OF_SUCCESS, err, "call-method(/instantiate-rtas)");

	hprintf("rtas located at 0x%x\n", res[1]);

}

uval
test_os(uval argc __attribute__ ((unused)),
	uval argv[] __attribute__ ((unused)))
{
	root_props();
	tree_walk();
	rtas();
	console();
	return 0;
}
