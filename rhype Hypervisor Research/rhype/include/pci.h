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

#ifndef _PCI_H
#define _PCI_H

#include <config.h>
#include <types.h>

/* Type 00h predefined header */
#define PCI_H0_VENDOR_ID_16	0x00	/* required */
#define PCI_H0_DEVICE_ID_16	0x02	/* required */

#define PCI_H0_COMMAND_16	0x04	/* required */
#define PCI_H0_COMMAND_IO_SPACE		0x0001
#define PCI_H0_COMMAND_MEMORY_SPACE	0x0002
#define PCI_H0_COMMAND_BUS_MASTER	0x0004
#define PCI_H0_COMMAND_SPECIAL_CYCLES	0x0008
#define PCI_H0_COMMAND_MW_AND_IE	0x0010	/* Mem. Write & Inval. Enable */
#define PCI_H0_COMMAND_VGA_PALETTE	0x0020
#define PCI_H0_COMMAND_PARITY		0x0040
#define PCI_H0_COMMAND_RESERVED		0x0080	/* Wait? address stepping? */
#define PCI_H0_COMMAND_SERR_ENABLE	0x0100
#define PCI_H0_COMMAND_FAST_BACK2BACK	0x0200
#define PCI_H0_COMMAND_INTERRUPT_DIS	0x0400
#define PCI_H0_STATUS_16	0x06	/* required */
#define PCI_H0_STATUS_RESERVED_0	0x0007
#define PCI_H0_STATUS_INTERRUPT_STATUS	0x0008
#define PCI_H0_STATUS_CAPABILITIES_LIST	0x0010
#define PCI_H0_STATUS_66MHZ_CAPABLE	0x0020
#define PCI_H0_STATUS_RESERVED_6	0x0040
#define PCI_H0_STATUS_FAST_BACK2BACK	0x0080
#define PCI_H0_STATUS_MASTER_DATA_PARI	0x0100
#define PCI_H0_STATUS_DEVSEL_MASK	0x0600
#define PCI_H0_STATUS_DEVSEL_FAST	0x0000
#define PCI_H0_STATUS_DEVSEL_MEDIUM	0x0200
#define PCI_H0_STATUS_DEVSEL_SLOW	0x0400
#define PCI_H0_STATUS_SIG_TARGET_ABORT	0x0800
#define PCI_H0_STATUS_RECV_TARGET_ABORT	0x1000
#define PCI_H0_STATUS_REVC_MASTER_ABORT	0x2000
#define PCI_H0_STATUS_SIG_SYSTEM_ERROR	0x4000
#define PCI_H0_STATUS_DETECTED_PARITY	0x8000

#define PCI_H0_REVISION_ID_8	0x08	/* required */
#define PCI_H0_CLASS_CODE_24	0x09	/* required */

#define PCI_H0_CACHE_LINESZ_8	0x0c
#define PCI_H0_LATENCY_TIMER_8	0x0d
#define PCI_H0_HEADER_TYPE_8	0x0e	/* required */
#define PCI_H0_HEADER_TYPE_NORMAL	0x00
#define PCI_H0_HEADER_TYPE_BRIDGE	0x01
#define PCI_H0_HEADER_TYPE_CARDBUS	0x02
#define PCI_H0_BIST_8		0x0f
#define PCI_H0_BIST_COMPLET_CODE_MASK	0x0f
#define PCI_H0_BIST_RESERVED		0x30
#define PCI_H0_BIST_START		0x40
#define PCI_H0_BIST_CAPABLE		0x80

#define PCI_H0_BASE_REGS_192	0x10

#define PCI_H0_CARDBUS_CIS_PTR_32 0x28

#define PCI_H0_SUBSYSTEM_VENDOR_ID_16 0x2c
#define PCI_H0_SUBSYSTEM_ID_16	0x2e

#define PCI_H0_EXP_ROM_BASE_ADDR_32 0x30

#define PCI_H0_CAPABILITIES_PTR_8 0x34

#define PCI_H0_INTERRUPT_LINE_8	0x3c
#define PCI_H0_INTERRUPT_PIN_8	0x3d
#define PCI_H0_MIN_GNT_8	0x3e
#define PCI_H0_MAX_LAT_8	0x3f

#endif /* _PCI_H */
