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
#include <test.h>
#include <hcall.h>

#define MAX_CRQ_SLOTS 64
static struct crq_maintenance cm[MAX_CRQ_SLOTS];

static sval
crq_transfer(struct crq_maintenance *cm, uval32 liobn, uval32 lpid, uval role)
{
	sval rc;
	uval rets[3];

	rc = hcall_resource_transfer(rets, INTR_SRC, liobn, 0, 0, lpid);
	assert(rc == H_Success,
	       "could not transfer CRQ (0x%x) to lpid=0x%x: err=%d\n",
	       liobn, lpid, (int)rc);
	if (H_Success != rc) {
		/* failure */
		if (ROLE_SERVER == role) {
			cm->cm_owner_server = 0;
		} else {
			cm->cm_owner_client = 0;
		}
	}
	return rc;
}

struct crq_maintenance *
crq_create(uval lpid, uval role, uval dma_sz)
{
	uval i = 0;
	struct crq_maintenance *res = NULL;

	while (i < MAX_CRQ_SLOTS) {
		if (ROLE_SERVER == role) {
			if (0 == cm[i].cm_owner_server) {
				cm[i].cm_owner_server = lpid;
				res = &cm[i];
				break;
			}
		} else {
			if (0 == cm[i].cm_owner_client) {
				cm[i].cm_owner_client = lpid;
				res = &cm[i];
				break;
			}
		}
		i++;
	}

	if (NULL != res) {
		uval rc;

		res->cm_slot = i;
		assert((0 == res->cm_liobn_server && 0 == res->cm_liobn_client)
		       || (0 != res->cm_liobn_server &&
			   0 != res->cm_liobn_client),
		       "crq maintenanc structure is in bad state.\n");
		/* have liobn values assigned to it already? */
		if (0 == res->cm_liobn_server && 0 == res->cm_liobn_server) {
			uval rets[3];
			uval32 liobn;

			/* need to acquire the liobn numbers */
			rc = hcall_vio_ctl(rets, HVIO_ACQUIRE, HVIO_CRQ,
					   dma_sz);
			assert(rc == H_Success, "could not allocate CRQ\n");

			if (rc == H_Success) {
				dma_sz = rets[2];
				if (dma_sz == 0) {
					/* out of memory ? release
					 * everything */
					rc = hcall_vio_ctl(NULL, HVIO_RELEASE,
							   rets[0], 0);
					assert(rc == H_Success,
					       "could not release CRQ for "
					       "1st partition\n");
					rc = hcall_vio_ctl(NULL, HVIO_RELEASE,
							   rets[1], 0);
					assert(rc == H_Success,
					       "could not release CRQ for "
					       "2nd partition\n");
				} else {
					res->cm_liobn_server = rets[0];
					res->cm_liobn_client = rets[1];
					res->cm_dma_sz = dma_sz;

					/* I am claiming the server
					 * side first */
					if (ROLE_SERVER == role) {
						liobn = res->cm_liobn_server;
					} else {
						liobn = res->cm_liobn_client;
					}

					if (lpid != H_SELF_LPID) {
						/* need to transfer
						 * this resource
						 * here */
						rc = crq_transfer(res, liobn,
								  lpid, role);
					}
					if (H_Success != rc) {
						res = NULL;
					}
				}
			}
		} else {
			uval32 liobn;

			/*
			 * The vlan has already been acquired, so now
			 * let me transfer it to the partition that's
			 * supposed to use it
			 */
			if (ROLE_SERVER == role) {
				liobn = res->cm_liobn_server;
			} else {
				liobn = res->cm_liobn_client;
			}

			/* need to transfer this resource here */
			rc = crq_transfer(res, liobn, lpid, role);
			if (H_Success != rc) {
				res = NULL;
			}
		}
	}

	return res;
}

/* Should be called upon destruction of a partition - not tested */
void
crq_release(uval32 lpid)
{
	uval i = 0;

	/*
	 * Drop all ownership claims of CRQ resources that this
	 * partiton used to own
	 */
	while (i < MAX_CRQ_SLOTS) {
		uval found = 0;

		/* 
		 * in any case, leave the liobn values untouched!! 
		 * cannot re-claim the resource.
		 */
		if (lpid == cm[i].cm_owner_server) {
			cm[i].cm_owner_server = 0;
			found = 1;
		} else if (lpid == cm[i].cm_owner_client) {
			cm[i].cm_owner_client = 0;
			found = 1;
		}

		/*
		 * CRQ does not have any owner anymore? Completely
		 * release it then.
		 */
		if (1 == found &&
		    0 == cm[i].cm_owner_client && 0 == cm[i].cm_owner_server) {
			uval rc;

			rc = hcall_vio_ctl(NULL, HVIO_RELEASE,
					   cm[i].cm_liobn_server, 0);
			assert(rc == H_Success,
			       "could not release CRQ for 1st partition\n");
			rc = hcall_vio_ctl(NULL, HVIO_RELEASE,
					   cm[i].cm_liobn_client, 0);
			assert(rc == H_Success,
			       "could not release CRQ for 2nd partition\n");
			cm[i].cm_liobn_server = 0;
			cm[i].cm_liobn_client = 0;
		}
		i++;
	}
}
