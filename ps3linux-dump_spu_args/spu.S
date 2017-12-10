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

.text

start:

	ila		$2, 0x3dfa0
	lr		$sp, $2

	ila		$80, 0x3e000
	lr		$81, $3

	# store SPU args in LS

	stqd		$7, 0($80)
	stqd		$8, 0x10($80)
	stqd		$9, 0x20($80)
	stqd		$10, 0x30($80)
	stqd		$11, 0x40($80)
	stqd		$12, 0x50($80)

	lr		$3, $80
	lr		$4, $81
	il		$5, 0x60
	il		$6, 0x7
	il		$7, 0x20
	brsl		$lr, 0x10	# mfc_dma_xfer

	il		$3, 0x7
	brsl		$lr, 0x28	# mfc_dma_wait

	stop		0x666		# our evil stop code :)

/*
 * r3 - LSA
 * r4 - EA
 * r5 - size
 * r6 - tag
 * r7 - cmd
 */
mfc_dma_xfer:

	wrch		$ch16, $3
	wrch		$ch17, $4
	shlqbyi		$4, $4, 4
	wrch		$ch18, $4
	wrch		$ch19, $5
	wrch		$ch20, $6
	wrch		$ch21, $7

	bi		$lr

/*
 * r3 - tag
 */
mfc_dma_wait:

	il		$2, 0
	nop		$127
	hbra		2f, 1f
	wrch		$ch23, $2

1:

	rchcnt		$2, $ch23
	ceqi		$2, $2, 1
	nop		$127
	nop		$127
	nop		$127
	nop		$127
	nop		$127

2:

	brz		$2, 1b
	hbr		3f, $lr
	rdch		$2, $ch24
	il		$2, 1
	shl		$2, $2, $3
	wrch		$ch22, $2
	il		$2, 2
	wrch		$ch23, $2
	rdch		$2, $ch24
	nop		$127

3:

	bi		$lr
