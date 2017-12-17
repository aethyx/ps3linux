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

#ifndef _HCALL_H
#define _HCALL_H

#include <config.h>
#include <types.h>

/* RPA 21.2.1.5.1 */
enum {
	MEM_ADDR = 0x1,
	MMIO_ADDR = 0x2,
	INTR_SRC = 0x3,
	DMA_WIND = 0x4,
	IPI_PORT = 0x5,
	PRIVATE = 0x6,		/* For unknown kinds of resources */
	MAX_TYPE = PRIVATE,
	INVALID = ~((uval)0)
};

/* Valid commands for hcall_pci_config */
enum {
	SET = 0x1,		/* OR this in with other values: */
	CREATE = 0x2,
	GRANT = 0x4,
	DMA_WIN = 0x6,
	CFG_SPC = 0x8,
	IOID = 0xa,
};

/* Valid command for hcall_eic_config */
enum {
	CONFIG  =	0x1,	/* arg values PIC specific */
	MAP_LINE=	0x2,	/* arg1 = lpid,
				 * arg4 = phys intr line,
				 * arg2 = cpu,
				 * arg3 = thread,
				 * ret[0] == isrc */
	CTL_LPAR=	0x3	/* arg1 = ctl partition id,
				 * arg2 = cpu,
				 * arg3 = thread */
};

enum {
	INVALID_LOGICAL_ADDRESS= (~((uval)0)),
};

extern sval hcall_create_partition(uval *retvals, uval laddr, uval laddr_size,
				   uval pinfo_offset);
extern sval hcall_destroy_partition(uval *retvals, uval lpid);

extern sval hcall_resource_transfer(uval *retvals, uval type,
				    uval addr_hi, uval addr_lo,
				    uval size, uval lpid);
extern sval hcall_start(uval *retvals, uval lpid, uval pc,
			uval r2, uval r3, uval r4, uval r5, uval r6, uval r7);

extern sval hcall_yield(uval *retvals, uval condition);
extern sval hcall_set_exception_info(uval *retvals, uval cpu, uval flags,
				     uval srrloc, uval exvec);
extern sval hcall_enter(uval *retvals, uval flags, uval idx, ...);
extern sval hcall_vm_map(uval *retvals, uval flags, uval pvpn, uval ptel);
extern sval hcall_read(uval *retvals, uval flags, uval idx);
extern sval hcall_remove(uval *retvals, uval flags, uval pte_index, uval avpn);
extern sval hcall_clear_mod(uval *retvals, uval flags, uval pte_index);
extern sval hcall_clear_ref(uval *retvals, uval flags, uval pte_index);
extern sval hcall_protect(uval *retvals, uval flags,
			  uval pte_index, uval avpn);
extern sval hcall_get_lpid(uval *retvals);
extern sval hcall_set_sched_params(uval *retvals, uval lpid,
				   uval cpu, uval required, uval desired);
extern sval hcall_yield_check_regs(uval lpid, uval regsave[]);
extern sval hcall_put_term_buffer(uval *retvals, uval channel,
				  uval length, uval buffer);
extern sval hcall_get_term_char(uval *retvals, uval idx);
extern sval hcall_put_term_char(uval *retvals, uval idx, uval count, ...);
extern sval hcall_register_vterm(uval *retvals, uval ua, uval plpid, uval pua);
extern sval hcall_vterm_partner_info(uval *retvals, uval ua, uval plpid,
				     uval pua, uval lpage);
extern sval hcall_free_vterm(uval *retvals, uval uaddr);

extern sval hcall_thread_control(uval *retvals, uval flags,
				 uval thread_num, uval start_addr);
extern sval hcall_cede(uval *retvals);
extern sval hcall_page_init(uval *retvals, uval flags,
			    uval destination, uval source);
extern sval hcall_set_asr(uval *retvals, uval value);	/* ISTAR only. */
extern sval hcall_asr_on(uval *retvals);	/* ISTAR only. */
extern sval hcall_asr_off(uval *retvals);	/* ISTAR only. */
extern sval hcall_eoi(uval *retvals, uval xirr);
extern sval hcall_cppr(uval *retvals, uval cppr);
extern sval hcall_ipi(uval *retvals, uval sn, uval mfrr);
extern sval hcall_ipoll(uval *retvals, uval sn);
extern sval hcall_xirr(uval *retvals);
extern sval hcall_logical_ci_load_64(uval *retvals, uval size,
				     uval addrAndVal);
extern sval hcall_logical_ci_store_64(uval *retvals, uval size,
				      uval addr, uval value);
extern sval hcall_logical_cache_load_64(uval *retvals, uval size,
					uval addrAndVal);
extern sval hcall_logical_cache_store_64(uval *retvals, uval size,
					 uval addr, uval value);
extern sval hcall_logical_icbi(uval *retvals, uval addr);
extern sval hcall_logical_dcbf(uval *retvals, uval addr);
extern sval hcall_set_dabr(uval *retvals, uval dabr);
extern sval hcall_hypervisor_data(uval *retvals, sval64 control);
extern sval hcall_create_msgq(uval *retvals, uval lbase,
			      uval size, uval flags);
extern sval hcall_send_async(uval *retvals, uval dest,
			     uval arg1, uval arg2, uval arg3, uval arg4);
extern sval hcall_real_to_logical(uval *retvals, uval raddr);

/* Hidden */
extern sval hcall_set_xive(uval *retvals, uval32 intr,
			   uval32 serv, uval32 prio);
extern sval hcall_get_xive(uval *retvals, uval32 intr);
extern sval hcall_interrupt(uval *retvals, uval32 intr, uval32 set);
extern sval hcall_pci_config_read(uval *retvals, uval caddr, uval buid,
				  uval size);
extern sval hcall_pci_config_write(uval *retvals, uval caddr, uval buid,
				   uval size, uval val);
extern sval hcall_pci_config(uval *retvals, uval command, uval buid,
			     uval arg1, uval arg2, uval arg3);

extern sval hcall_eic_config(uval *retvals, uval command,
			     uval arg1, uval arg2, uval arg3, uval arg4);

extern sval hcall_mem_define(uval *retvals, uval type, uval addr, uval size);

extern sval hcall_grant_logical(uval *retvals, uval flags,
				uval logical_hi, uval logical_lo, uval length,
				uval unit_address);
extern sval hcall_accept_logical(uval *retvals, uval cookie);
extern sval hcall_rescind_logical(uval *retvals, uval flags, uval cookie);
extern sval hcall_htab(uval *retvals, uval lpid, uval htab_size);

enum {
	H_BREAKPOINT	  = 0x1000,	/* Causes HV breakpoint */
	H_SET_DEBUG_LEVEL = 0x2000,	/* Set debug level for debug printfs */
	H_THINWIRE_RESET  = 0x3000,	/* Attempt to recover thinwire */
	H_PROBE_REG	  = 0x4000,	/* Probe a partitions registers */
};
extern sval hcall_debug(uval *retvals, uval cmd, uval arg1, uval arg2,
			uval arg3, uval arg4);


enum {
	H_GET_PINFO	  = 0x1000,	/* Load pinfo to LA arg1 */
	H_GET_MEM_INFO	  = 0x2000,	/* Return descriptor of the "arg1"'th
					   present mem area
					   indexed "arg1" in ret[0], ret[1] */
};
extern sval hcall_lpar_info(uval *retvals, uval cmd, uval arg1, uval arg2,
			    uval arg3, uval arg4);


extern sval hcall_vio_ctl(uval *retvals, uval cmd, uval arg1, uval arg2);

#endif /* ! _HCALL_H */
