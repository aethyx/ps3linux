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

/*
 * These are services particular to poweprc 32/64
 */

#include <ofh.h>

struct ofh_srvc_s ofh_isa_srvc[] = {
	/* Document Section 8.5.1 Real-Mode physical memory ... */
	{ .ofs_name = "alloc-real_mem", .ofs_func = ofh_nosup },

	/* Document Section 8.5.2 Virtual address translation ... */
	{ .ofs_name = "map", .ofs_func = ofh_nosup },
	{ .ofs_name = "unmap", .ofs_func = ofh_nosup },
	{ .ofs_name = "translate", .ofs_func = ofh_nosup },

	/* Document Section 11.3 Client Interface Services */
	{ .ofs_name = "start-cpu", .ofs_func = ofh_start_cpu },
	{ .ofs_name = "stop-self", .ofs_func = ofh_stop_self },
	{ .ofs_name = "idle-self", .ofs_func = ofh_idle_self },
	{ .ofs_name = "resume-cpu", .ofs_func = ofh_resume_cpu },
	{ .ofs_name = NULL, .ofs_func = NULL}
};

