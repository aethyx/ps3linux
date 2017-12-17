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
 *
 */

#include <config.h>
#include <hcall.h>
#include <lib.h>
#include <ofh.h>
#include <util.h>
#include <of_devtree.h>

extern sval ofh_start(struct ofh_args_s *);
extern void _start(void);

/*
 * 6.3.1 Access to the client interface functions
 * This is the spec'd maximum
 */
#define OFH_MAXSRVCLEN 31

static uval ofh_maxsrvclen;

extern sval
debug(const char *fmt, ...);

sval
debug(const char *fmt, ...)
{
	sval sz;
	va_list ap;
	char buf[512];
	va_start(ap, fmt);
	sz = vsnprintf(buf, 512, fmt, ap);
	va_end(ap);
	ofh_cons_write(buf, sz, &sz);

	return sz;
}



void
assprint(const char *expr, const char *file, int line, const char *fmt, ...)
{
	char a[15];

	a[0]  = '\n';
	a[1]  = '\n';
	a[2]  = 'O';
	a[3]  = 'F';
	a[4]  = 'H';
	a[5]  = ':';
	a[6]  = 'A';
	a[7]  = 'S';
	a[8]  = 'S';
	a[9]  = 'E';
	a[10] = 'R';
	a[11] = 'T';
	a[12] = '!';
	a[13] = '\n';
	a[14] = '\n';

	uval actual;
	uval t = 1;
	volatile uval *tp = &t;

	(void)expr; (void)file; (void)line; (void)fmt;

	ofh_cons_write(a, sizeof (a), &actual);

	/* maybe I can break out of this loop manually (like with a
	 * debugger) */
	while (*tp) {
		continue;
	}
}

/*
 * we use elf hash since it is pretty standard
 */
static uval
of_hash(const char *s)
{
	uval hash = 0;
	uval hnib;

	if (s != NULL) {
		while (*s != '\0') {
			hash = (hash << 4) + *s++;
			hnib = hash & 0xf0000000UL;
			if (hnib != 0) {
				hash ^= hnib >> 24;
			}
			hash &= ~hnib;
		}
	}
	return hash;
}

static void
ofh_service_init(uval b)
{
	uval sz;
	int i;
	int j = 0;
	struct ofh_srvc_s *o;
	struct ofh_srvc_s *ofs[] = {
		DRELA(&ofh_srvc[0], b),
		DRELA(&ofh_isa_srvc[0], b),
		NULL
	};

	j = 0;
	while (ofs[j] != NULL) {
		/* find the maximum string length for services */
		o = &ofs[j][0];
		while (o->ofs_name != NULL) {
			const char *n;

			n = DRELA(&o->ofs_name[0], b);
			/* fix it up so we don't have to fix it anymore */
			o->ofs_name = n;

			sz = strlen(n);
			if (sz > *DRELA(&ofh_maxsrvclen, b)) {
				*DRELA(&ofh_maxsrvclen, b) = sz;
			}
			o->ofs_hash =
				of_hash(n);
			++i;
			++o;
		}
		++j;
	}
}


static void
ofh_cpu_init(ofdn_t chosen, uval b)
{
	static struct ofh_ihandle_s _ih_cpu_0;
	void *mem = ofd_mem(b);
	uval ih = DRELA((uval)&_ih_cpu_0, b);
	struct ofh_ihandle_s *ihp = (struct ofh_ihandle_s *)ih;
	const char *cpu_type = DRELA((const char*)"cpu",b);

	ofdn_t cpu = ofd_node_find_by_prop(mem, OFD_ROOT,
					   DRELA((const char*)"device_type",b),
					   cpu_type, sizeof(cpu_type));
	ihp->ofi_node = cpu;
	ofd_prop_add(mem, chosen, DRELA((const char *)"cpu", b),
		     &ih, sizeof (ih));
}
static sval
mmu_translate(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	/* FIXME: need a little more here */
	nargs = nargs;
	nrets = nrets;
	argp = argp;
	retp = retp;
	b = b;
	return OF_SUCCESS;
}

static void
ofh_mmu_init(ofdn_t chosen, uval b)
{
	static struct ofh_methods_s _mmu_methods[] = {
		{ "translate", mmu_translate },
		{ NULL, NULL},
	};
	static struct ofh_ihandle_s _ih_mmu = {
		.ofi_methods = _mmu_methods,
	};
	void *mem = ofd_mem(b);
	uval ih = DRELA((uval)&_ih_mmu, b);

	ofd_prop_add(mem, chosen, DRELA((const char *)"mmu", b),
		     &ih, sizeof (ih));
}

static void
ofh_chosen_init(uval b)
{
	ofdn_t ph;
	void *mem = ofd_mem(b);

	ph = ofd_node_find(mem, DRELA((const char *)"/chosen", b));

	ofh_vty_init(ph, b);
	ofh_cpu_init(ph, b);
	ofh_mmu_init(ph, b);
}

static void
ofh_options_init(uval b)
{
	void *mem = ofd_mem(b);
	ofdn_t options;
	uval32 size = 1 << 20;
	uval32 base = (uval32)DRELA(&ofh_start,b);
	char buf[20];
	int i;


	/* fixup the ihandle */
	options = ofd_node_find(mem,
				DRELA((const char *)"options", b));

	i = snprintf(buf,20, "0x%x",base);
	ofd_prop_add(mem, options, DRELA((const char *)"real-base", b),
		     buf, i);

	i = snprintf(buf,20, "0x%x",size);
	ofd_prop_add(mem, options, DRELA((const char *)"real-size", b),
		     buf, i);
}




extern char __bss_start[];
extern char _end[];

static void
ofh_init(uval b)
{
	uval sz = (uval)_end - (uval)__bss_start;
	/* clear bss */
	memset(__bss_start + b, 0, sz);

	ofh_service_init(b);
	ofh_chosen_init(b);
	ofh_rtas_init(b);
	ofh_options_init(b);
}

static ofh_func_t *
ofh_lookup(const char *service, uval b)
{
	int j;
	uval hash;
	struct ofh_srvc_s *o;
	struct ofh_srvc_s *ofs[] = {
		DRELA(&ofh_srvc[0], b),
		DRELA(&ofh_isa_srvc[0], b),
		NULL
	};
	uval sz;

	sz = *DRELA(&ofh_maxsrvclen, b);

	if (strnlen(service, sz + 1) > sz) {
		return NULL;
	}

	hash = of_hash(service);

	j = 0;
	while (ofs[j] != NULL) {
		/* yes this could be quicker */
		o = &ofs[j][0];
		while (o->ofs_name != NULL) {
			if (o->ofs_hash == hash) {
				const char *n = o->ofs_name;
				if (strcmp(service, n) == 0) {
					return o->ofs_func;
				}
			}
			++o;
		}
		++j;
	}
	return NULL;
}

sval
ofh_nosup(uval nargs __attribute__ ((unused)),
	  uval nrets __attribute__ ((unused)),
	  sval argp[] __attribute__ ((unused)),
	  sval retp[] __attribute__ ((unused)),
	  uval b __attribute__ ((unused)))
{
	return OF_FAILURE;
}

sval
ofh_test_method(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	if (nargs == 2) {
		if (nrets == 1) {
			sval *ap = DRELA(&ofh_active_package, b);
			uval service = (sval)argp[0];
			const char *method = (const char *)argp[1];
			sval *stat = &retp[0];

			(void)ap; (void)service; (void)method;

			*stat = 0;
			/* we do not do this yet */
			return OF_FAILURE;
		}
	}
	return OF_FAILURE;
}
extern uval32 _ofh_inited[0];
extern uval _ofh_lastarg[0];

sval
ofh_handler(struct ofh_args_s *args, uval b)
{
	uval32 *inited = (uval32 *)DRELA(&_ofh_inited[0],b);
	uval *lastarg = (uval *)DRELA(&_ofh_lastarg[0],b);
	ofh_func_t *f;

	if (*inited == 0) {
		ofh_init(b);
		*inited = 1;
	}

	*lastarg = (uval)args;

	f = ofh_lookup(args->ofa_service, b);

	if (f == ((ofh_func_t *)~0UL)) {
		/* do test */
		if (args->ofa_nargs == 1) {
			if (args->ofa_nreturns == 1) {
				char *name = (char *)args->ofa_args[0];
				if (ofh_lookup(name, b) != NULL) {
					args->ofa_args[args->ofa_nargs] =
						OF_SUCCESS;
					return OF_SUCCESS;
				}
			}
		}
		return OF_FAILURE;

	} else if (f != NULL) {
		return leap(args->ofa_nargs,
			    args->ofa_nreturns,
			    args->ofa_args,
			    &args->ofa_args[args->ofa_nargs],
			    b, (uval)f);
	}
	return OF_FAILURE;
}

/*
 * The following code exists solely to run the handler code standalone
 */
void
_start(void)
{
	sval ret;
	struct ofh_args_s *args;
	uval argsz;
	uval of_stdout;
	uval ihandle;
	char buf[1024];

	argsz = sizeof (struct ofh_args_s);
	argsz += sizeof (args->ofa_args[0]) * 10;
	args = alloca(argsz);


	args->ofa_service	= "finddevice";
	args->ofa_nargs		= 1;
	args->ofa_nreturns	= 1;
	args->ofa_args[0]	= (uval)"/";
	args->ofa_args[1]	= -1;
	ret = ofh_start(args);

	if (ret == OF_SUCCESS) {
		args->ofa_service	= "finddevice";
		args->ofa_nargs		= 1;
		args->ofa_nreturns	= 1;
		args->ofa_args[0]	= (uval)"/chosen";
		args->ofa_args[1]	= -1;
		ret = ofh_start(args);
	}

	if (ret == OF_SUCCESS) {
		uval phandle = args->ofa_args[1];

		args->ofa_service	= "getprop";
		args->ofa_nargs		= 4;
		args->ofa_nreturns	= 1;
		args->ofa_args[0]	= phandle;
		args->ofa_args[1]	= (uval)"stdout";
		args->ofa_args[2]	= (uval)&of_stdout;
		args->ofa_args[3]	= sizeof(of_stdout);
		args->ofa_args[4]	= -1;
		ret = ofh_start(args);
	}

	ihandle = *(uval *)args->ofa_args[2];

	if (ret == OF_SUCCESS) {
		/* instance to path */
		args->ofa_service	= "instance-to-path";
		args->ofa_nargs		= 3;
		args->ofa_nreturns	= 1;
		args->ofa_args[0]	= ihandle;
		args->ofa_args[1]	= (uval)buf;
		args->ofa_args[2]	= sizeof (buf);
		args->ofa_args[3]	= -1;
		ret = ofh_start(args);

	}

	if (ret == OF_SUCCESS) {
		/* open rtas */
		args->ofa_service	= "open";
		args->ofa_nargs		= 1;
		args->ofa_nreturns	= 1;
		args->ofa_args[0]	= (uval)"/rtas";
		ret = ofh_start(args);
		if (ret == OF_SUCCESS) {
			uval ir = args->ofa_args[1];
			args->ofa_service	= "call-method";
			args->ofa_nargs		= 3;
			args->ofa_nreturns	= 2;
			args->ofa_args[0]	= (uval)"instantiate-rtas";
			args->ofa_args[1]	= ir;
			args->ofa_args[2]	= (uval)buf;

			ret = ofh_start(args);
		}
	}

	if (ret == OF_SUCCESS) {
		const char msg[] = "This is a test";
		uval msgsz = sizeof(msg) - 1; /* Includes \0 */

		args->ofa_service	= "write";
		args->ofa_nargs		= 3;
		args->ofa_nreturns	= 1;
		args->ofa_args[0]	= ihandle;
		args->ofa_args[1]	= (uval)msg;
		args->ofa_args[2]	= msgsz;
		args->ofa_args[3]	= -1;
		ret = ofh_start(args);
	}

}
