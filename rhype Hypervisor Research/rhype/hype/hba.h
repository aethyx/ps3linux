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
#ifndef _HBA_H
#define _HBA_H

#include <types.h>
#include <dlist.h>
#include <lib.h>

enum {
	MAX_USERS = 16,
};

struct hba {

	struct dlist hba_list;
	uval users[MAX_USERS];
	uval buid;
	uval32 ioid;

	struct {
		uval start;
		uval size;
	} config_space;

	struct {
		uval32 liobn;
		uval64 start;
		uval32 size;
	} dma_win;
};
extern lock_t hba_lock;

extern sval hba_init(uval ofd);

extern struct hba *hba_get(uval buid);

extern sval hba_define(uval buid);

extern uval hba_config_read(uval lpid, uval addr, uval buid,
			    uval size, uval *val);
extern uval hba_config_write(uval lpid, uval addr, uval buid,
			     uval size, uval val);

static inline uval
hba_owner(struct hba *hba, uval lpid)
{
	int i = 0;

	while (i < MAX_USERS) {
		if (hba->users[i++] == lpid) {
			return 1;
		}
	}
	return 0;
}

#endif /* ! _HBA_H */
