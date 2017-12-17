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
 * A console driver for test OSs.
 */

#include <test.h>
#include <hcall.h>
#include <mmu.h>

//#define USE_VTERM

#if defined(USE_VTERM) && !defined(RELOADER)
#define VTERM
#endif

#ifdef VTERM
static uval
vty_service1(uval vty, uval tc)
{
	sval rc;
	uval w_rets[3];
	uval r_rets[3];
	uval remain = 0;

	for (;;) {
		rc = hcall_get_term_char(w_rets, vty);
		switch (rc) {
		default:
			assert(0, "wierd return\n");
			return 0;

		case  H_Parameter:
			return 0;

		case H_Closed:
			rc = hcall_free_vterm(NULL, vty);
			assert(rc == H_Success,
			       "could not free vty: 0x%lx\n", vty);
			return 0;

		case H_Success:
			if (w_rets[0] > 0) {
				rc = hcall_put_term_char(NULL, tc, w_rets[0],
							 w_rets[1], w_rets[2]);
				assert(rc == H_Success,
				       "could not write for tc: 0x%lx\n", vty);
			}
			break;
		}

		if (remain) {
			rc = H_Success;
			remain = 0;
		} else {
			rc = hcall_get_term_char(r_rets, tc);
			assert(rc == H_Success,
			       "could not read tc: 0x%lx\n", tc);
		}
		if (rc == H_Success) {
			if (r_rets[0] > 0) {
				rc = hcall_put_term_char(NULL, vty,
							 r_rets[0],
							 r_rets[1],
							 r_rets[2]);
				switch (rc) {
				default:
					assert(0, "no write for vty: 0x%lx\n",
					       vty);
					return 0;
				case H_Busy:
					remain = 1;
				case H_Success:
					break;
				}
			}
		}
		if (r_rets[0] == 0 && w_rets[0] == 0) {
			return 1;
		}
	}
}

static void
vterm_partner_info(uval server, uval client, uval lpid)
{
	static uval64 page[PGSIZE / sizeof (uval64)]
		__attribute__ ((aligned(PGSIZE)));
	uval64 *b = page;
	sval rc;
	uval plpid = ~0UL;
	uval pua = ~0UL;
	uval found = 0;

	hprintf("%s: server: 0x%lx, client: 0x%lx, lpid: 0x%lx\n",
		__func__, server, client, lpid);
	hprintf("testing: hcall_vterm_partner_info:\n");
	do {
		char *c;
		const char *pre;

		rc = hcall_vterm_partner_info(NULL, server, plpid, pua,
					      (uval)page);
		assert(rc == H_Success, "hcall failed\n");


		plpid = b[0];
		pua = b[1];
		c = (char *)&b[2];
		
		if (plpid == lpid && pua == client) {
			pre = " **";
			found = 1;
		} else {
			pre = "   ";
		}
		hprintf("%s lpid: 0x%lx, uaddr: 0x%lx CLC: %s\n",
			pre, plpid, pua, c);
	} while (b[0] != ~0UL && b[1] != ~0UL);
	assert(found, "could not find specific client: 0x%lx\n", client);
}
#endif

uval
vterm_create_server(void)
{
	sval rc;
	uval rets[3];
	rc = hcall_vio_ctl(rets, HVIO_ACQUIRE, HVIO_VTERM, PGSIZE);
	assert(rc == H_Success, "could not allocate server vterm\n");
	if (rc == H_Success) {
		return rets[0];
	}
	return 0;
}

uval
vterm_create(struct partition_status *ps, uval *server, uval *client)
{
#ifndef VTERM
	(void)ps;
	*server = 0;
	*client = 0;
	return 1;
#else
	uval rets[3];
	sval rc;
	uval lpid = ps->lpid;
	uval s;
	uval c;

	s = vterm_create_server();
	if (s == 0) {
		return 0;
	}

	/* dma size of zero tags this as the client side */
	rc = hcall_vio_ctl(rets, HVIO_ACQUIRE, HVIO_VTERM, 0);
	assert(rc == H_Success, "could not allocate client vterm\n");

	c = rets[0];

	if (lpid != H_SELF_LPID) {
		/* need to transfer this resource here */
		rc = hcall_resource_transfer(rets, INTR_SRC, c, 0, 0, lpid);
		assert(rc == H_Success, "could not transfer vterm\n");
		c = rets[0];
	}

	vterm_partner_info(s, c, lpid);

	hprintf("server: 0x%lx client: 0x%lx\n", s, c);
	rc = hcall_register_vterm(NULL, s, lpid, c);

	assert(rc == H_Success, "could not register vterm: 0x%lx\n", s);
	ps->vterm = s;

	*server = s;
	*client = c;
	
	return 1;
#endif
}

void
vterm_service(struct partition_status *ps, uval sz)
{
#ifndef VTERM
	(void)ps;
	(void)sz;
	return;
#else
	uval i;
	for (i = 0; i < sz; ++i) {
		/* still want to blead the inactive */
		if (ps[i].vterm > 0) {
			if (!vty_service1(ps[i].vterm, 2 + i)) {
				ps[i].vterm = 0;
			}
		}
	}
#endif
}


