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

#include <ofh.h>
#include <lib.h>
#include <util.h>

static struct ofh_ihandle_s _ih_vty_0 = {
	.ofi_read = ofh_cons_read,
	.ofi_write = ofh_cons_write,
};

void
ofh_vty_init(ofdn_t chosen, uval b)
{
	void *mem = ofd_mem(b);
	uval ih = DRELA((uval)&_ih_vty_0, b);
	struct ofh_ihandle_s *ihp = (struct ofh_ihandle_s *)ih;
	ofdn_t vty;
	sval ret;
	uval32 chan;


	/* fixup the ihandle */
	vty = ofd_node_find(mem,
			    DRELA((const char *)"/vdevice/vty", b));
	ihp->ofi_node = vty;

	ret = ofd_getprop(mem, vty, DRELA((const char *)"reg", b),
			&chan, sizeof (chan));
	if (ret != (sval)sizeof (chan)) {
		chan = 0;
	}
	ofh_cons_init(chan, b);

	ofd_prop_add(mem, chosen, DRELA((const char *)"stdout", b),
		     &ih, sizeof (ih));
	ofd_prop_add(mem, chosen, DRELA((const char *)"stdin", b),
		     &ih, sizeof (ih));
}


