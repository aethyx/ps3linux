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
#include <hcall.h>
#include <lpar.h>

union chpack {
	uval64 oct[2];
	uval32 quad[4];
	char c[16];
};

static uval ofh_cchan;

sval
ofh_cons_read(void *buf, uval count, uval *actual)
{
	sval rc;
	uval ret[5];
	/* FIXME:  We need to relocate ofh_ccchan, but against what? */
	/*         This is called via leap, but leap has not way of passing */
	/*         the base value through to let us do the relocation. */
	rc = hcall_get_term_char(ret, ofh_cchan);
	if (rc == H_Success && ret[0] > 0) {
		uval sz;

		sz = MIN(count, ret[0]);
		memcpy(buf, &ret[1], sz);

		*actual = sz;
	} else {
		*actual = 0;
	}
	return OF_SUCCESS;
}

sval
ofh_cons_write(const void *buf, uval count, uval *actual)
{
	const char *str = (const char *)buf;
	uval i;
	union chpack ch;
	sval ret;

	/* FIXME:  We need to relocate ofh_ccchan, but against what? */
	/*         This is called via leap, but leap has not way of passing */
	/*         the base value through to let us do the relocation. */

	for (i = 0; i < count; i++) {
		int m = i % sizeof(ch);
		ch.c[m] = str[i];
		if (m == sizeof(ch) - 1 || i == count - 1) {
			for (;;) {
				if (sizeof (uval) == sizeof (uval64)) {
					ret = hcall_put_term_char(NULL,
								  ofh_cchan,
								  m + 1,
								  ch.oct[0],
								  ch.oct[1]);
				} else {
					ret = hcall_put_term_char(NULL,
								  ofh_cchan,
								  m + 1,
								  ch.quad[0],
								  ch.quad[1],
								  ch.quad[2],
								  ch.quad[3]);
				}
				if (ret != H_Busy) {
					break;
				}
				hcall_yield(NULL, H_SELF_LPID);
			}
			if (ret != H_Success) {
				*actual = -1;
				return OF_FAILURE;
			}
		}
	}
	*actual = count;
	return OF_SUCCESS;
}

sval
ofh_cons_close(void)
{
	return OF_SUCCESS;
}

void
ofh_cons_init(uval chan, uval b)
{
	*DRELA(&ofh_cchan, b) = chan;
}
