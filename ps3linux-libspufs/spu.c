/*
 * Copyright (C) 2012 glevand <geoffrey.levand@mail.ru>
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdint.h>
#include <unistd.h>

#include "spu.h"

/*
 * spu_mfc_dma_xfer
 */
int spu_mfc_dma_xfer(struct spu_problem *problem, uint32_t lsa, uint64_t ea,
	uint16_t size, uint16_t tag, uint16_t class, uint16_t cmd)
{
	out_be32(&problem->mfc_lsa_W, lsa);
	out_be64(&problem->mfc_ea_W, ea);
	out_be32(&problem->mfc_union_W.by32.mfc_size_tag32, (size << 16) | tag);
	out_be32(&problem->mfc_union_W.by32.mfc_class_cmd32, (class << 16) | cmd);

	return in_be32(&problem->mfc_union_W.by32.mfc_class_cmd32) & 0x3;
}

/*
 * spu_mfc_dma_wait
 */
void spu_mfc_dma_wait(struct spu_problem *problem, uint16_t tag)
{
	while (1) {
		out_be32(&problem->dma_querymask_RW, MFC_TAGID_TO_TAGMASK(tag));
		out_be32(&problem->dma_querytype_RW, 0x2);

		if (in_be32(&problem->dma_tagstatus_R) & MFC_TAGID_TO_TAGMASK(tag))
			break;

		usleep(100000);
	}
}
