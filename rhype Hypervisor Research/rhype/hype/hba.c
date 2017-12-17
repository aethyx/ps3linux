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

#include <config.h>
#include <hba.h>
#include <mmu.h>
#include <lib.h>
#include <os.h>
#include <pmm.h>
#include <logical.h>
#include <objalloc.h>
#include <hba.h>
void __hba_init(void);

static struct dlist hba_list;
lock_t hba_lock;

void
__hba_init(void)
{
	dlist_init(&hba_list);
	lock_init(&hba_lock);
}

struct hba *
hba_get(uval buid)
{
	struct dlist *curr = dlist_next(&hba_list);

	while (curr != &hba_list) {
		struct hba *hba;
		hba = PARENT_OBJ(struct hba, hba_list, curr);

		if (buid == hba->buid) {
			lock_release(&hba_lock);
			return hba;
		}
		curr = dlist_next(&hba_list);
	}

	return 0;
}

sval
hba_define(uval buid)
{
	struct hba *h = hba_get(buid);

	if (h) {
		return H_Parameter;
	}

	struct hba *hba;

	hba = halloc(sizeof (*hba));
	memset(hba, 0, sizeof (*hba));

	dlist_init(&hba->hba_list);
	hba->buid = buid;
	dlist_insert(&hba->hba_list, &hba_list);
	return 0;
}
