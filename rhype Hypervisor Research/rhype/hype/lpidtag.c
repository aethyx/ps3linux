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

/* Manage the tags used for the LPIDR register. These are hardware-recognized
 * values, unlike the plain "lpid" field. */

#include <lpidtag.h>
#include <bitmap.h>
#include <os.h>

static struct lbm *lpidtags;

void
lpidtag_acquire(struct os *os)
{
	os->po_lpid_tag = lbm_acquire(lpidtags);
	/* TODO: remove assert; this case just means we need to tlbia more */
	assert(os->po_lpid_tag != (uval16)-1, "couldn't reserve an LPID tag\n");
}

void
lpidtag_release(struct os *os)
{
	lbm_release(lpidtags, os->po_lpid_tag);
}

void
lpidtag_init(int num_lpidr_bits)
{
	lpidtags = lbm_alloc(1UL<<num_lpidr_bits);
	lbm_set_all_bits(lpidtags, 0);
}
