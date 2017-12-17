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

#include <func_instrument.h>
#include <lib.h>

/* use of non-zero values, since we cannot rely */
/* on a zero initialized BSS                    */

enum {
	trace_enabled = 1,
	trace_disabled = 2
};

static int trace_status = trace_disabled;

static int call_depth = 0;

char buf[512];

void
enable_func_tracing(void)
{
	trace_status = trace_enabled;
}

void
disable_func_tracing()
{
	trace_status = trace_disabled;
}

uval volatile prof_enter_cnt = 0;
uval volatile prof_exit_cnt = 0;

static volatile uval prof_cnt0 = 0;
static volatile uval prof_cnt1 = 0;

void
__cyg_profile_func_enter(void *this_fn, void *call_site)
{

	prof_enter_cnt++;
	if (trace_status == trace_enabled) {
		snprintf(buf, sizeof (buf) - 1,
			 "func_enter[%d] %ld %p:%p\n",
			 call_depth++, prof_cnt0++, this_fn, call_site);
		hputs(buf);
	}

	prof_enter_cnt--;
}

void
__cyg_profile_func_exit(void *this_fn, void *call_site)
{
	prof_exit_cnt++;
	if (trace_status == trace_enabled) {
		snprintf(buf, sizeof (buf) - 1,
			 "func_exit [%d] %ld %p:%p\n",
			 call_depth--, prof_cnt1++, this_fn, call_site);
		hputs(buf);
	}
	prof_exit_cnt--;
}
