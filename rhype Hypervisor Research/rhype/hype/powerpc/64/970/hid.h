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
 * Details of the HID
 *
 */
#ifndef _HID_H_
#define _HID_H_

#include <regs.h>

union hid0 {
	/* *INDENT-OFF* */
	struct HID0bits{
		uval64  _unused_0_8:	9;
		uval64  nap:		1;
		uval64	_unused_10:	1;
		uval64  dpm:		1;  /* Dynamic Power Management */
		uval64	_unused_12_14:	3;
		uval64  nhr:		1;  /* Not Hard Reset */
		uval64	inorder:	1;
		uval64	_reserved17:	1;
		uval64	tb_ctrl:	1;
		uval64	ext_tb_enb:	1;  /* tb is linked to external clock*/
		uval64	_unused_20_22:	3;
		uval64	hdice:		1;  /* HDEC enable */
		uval64  eb_therm:	1;  /* Enable ext thermal ints */
		uval64	_unused_25_30:	6;
		uval64	en_attn:	1;  /* Enable attn instruction */
		uval64	_unused_32_63:	32;
	} bits;
	/* *INDENT-ON* */
	uval64 word;
};

union hid1 {
	/* *INDENT-OFF* */
	struct HID1bits{
		uval64 bht_pm:		3; /* branch hist tbl prediction */
		uval64 en_ls:		1; /* enable link stack */
		uval64 en_cc:		1; /* enable count cache */
		uval64 en_ic:		1; /* enable inst cache */
		uval64 _reserved_6:	1;
		uval64 pf_mode:		2; /* prefetch mode */
		uval64 en_icbi:		1; /* enable forced icbi match mode */
		uval64 en_if_cach:	1; /* i-fetch cacheability control */
		uval64 en_ic_rec:	1; /* i-cache parity error recovery */
		uval64 en_id_rec:	1; /* i-dir parity error recovery */
		uval64 en_er_rec:	1; /* i-ERAT parity error recovery */
		uval64 ic_pe:		1;
		uval64 icd0_pe:		1;
		uval64 _reserved_16:	1;
		uval64 ier_pe:		1;
		uval64 en_sp_itw:	1;
		uval64 _reserver_19_63:	45;
	} bits;
	/* *INDENT-ON* */
	uval64 word;
};

union hid4 {
	/* *INDENT-OFF* */
	struct HID4bits{
		uval64	lpes0:		1;  /* LPES0 */
		uval64	rmlr12:		2;  /* RMLR 1:2 */
		uval64  lpid25:		4;  /* LPID 2:5 */
		uval64	rmor:		16; /* RMOR */
		uval64  rm_ci:		1;  /* HV RM Cache-inhibit */
		uval64  force_ai:	1;  /* Force alignment interrupt */
		uval64  _unused:	32;
		uval64	lpes1:		1;  /* LPES1 */
		uval64	rmlr0:		1;  /* RMLR 0 */
		uval64	_reserved:	1;
		uval64	dis_splarx:	1;  /* Disable spec. lwarx/ldarx */
		uval64	lg_pg_dis:	1;  /* Disable large page support */
		uval64	lpid01:		2;  /* LPID 0:1 */
	} bits;
	/* *INDENT-ON* */
	uval64 word;
};

union hid5 {
	/* *INDENT-OFF* */
	struct HID5bits {
		uval64  _reserved_0_31:	32;
		uval64  hrmor:		16;
		uval64	_reserver_48_49:2;
		uval64	_unused_50_55:	6;
		uval64	DCBZ_size:	1;
		uval64	DCBZ32_ill:	1;
		uval64  _unused_58_63:	6;
	} bits;
	/* *INDENT-ON* */
	uval64 word;
};
#endif /* #ifndef _HID_H_ */
