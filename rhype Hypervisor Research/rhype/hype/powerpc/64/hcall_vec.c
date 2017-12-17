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
 * Hypervisor call dispatching.
 *
 */

#include <config.h>
#include <lpar.h>
#include <hype.h>
#include <h_proto.h>

/* Convert hypervisor embedded ID to function-table index. */

#define HEID2X(hid) (((hid)-H_EMBEDDED_BASE)/4)

/*
 * hcall table
 */
const hcall_vec_t hcall_vec[] = {
	/* *INDENT-OFF* */
	[0]				= h_invalid,
        [H_REMOVE/4]			= h_remove,
      	[H_ENTER/4]			= h_enter,
      	[H_READ/4]			= h_read,
	[H_CLEAR_MOD/4]			= h_clear_mod,
	[H_CLEAR_REF/4]			= h_clear_ref,
	[H_PROTECT/4]			= h_protect,
	[H_GET_TCE/4]			= h_get_tce,
	[H_PUT_TCE/4]			= h_put_tce,
	[H_STUFF_TCE/4]			= h_stuff_tce,
	[H_SET_SPRG0/4]			= h_set_sprg0,
#ifdef HAS_DABR
	[H_SET_DABR/4]			= h_set_dabr,
#else /* HAS_DABR */
	[H_SET_DABR/4]			= h_invalid,
#endif /* HAS_DABR */
	[H_PAGE_INIT/4]			= h_page_init,
	[H_SET_ASR/4]			= h_set_asr,
	[H_ASR_ON/4]			= h_asr_on,
	[H_ASR_OFF/4]			= h_asr_off,
	[H_LOGICAL_CI_LOAD/4]		= h_logical_ci_load,
	[H_LOGICAL_CI_STORE/4]		= h_logical_ci_store,
	[H_LOGICAL_CACHE_LOAD/4]	= h_logical_cache_load,
	[H_LOGICAL_CACHE_STORE/4]	= h_logical_cache_store,
	[H_LOGICAL_ICBI/4]		= h_logical_icbi,
	[H_LOGICAL_DCBF/4]		= h_logical_dcbf,
	[H_GET_TERM_CHAR/4]		= h_get_term_char,
	[H_PUT_TERM_CHAR/4]		= h_put_term_char,
	[H_REAL_TO_LOGICAL/4]		= h_real_to_logical,
	[H_HYPERVISOR_DATA/4]		= h_hypervisor_data,
	[H_EOI/4]			= h_eoi,
	[H_CPPR/4]			= h_cppr,
	[H_IPI/4]			= h_ipi,
	[H_IPOLL/4]			= h_ipoll,
	[H_XIRR/4]			= h_xirr,
	[H_CEDE/4]			= h_cede,
	[H_CONFER/4]			= h_confer,
	[H_PROD/4]			= h_prod,
	[H_GET_PPP/4]			= h_get_ppp,
	[H_SET_PPP/4]			= h_set_ppp,
	[H_PURR/4]			= h_purr,
	[H_PIC/4]			= h_pic,
	[H_REG_CRQ/4]			= h_reg_crq,
	[H_FREE_CRQ/4]			= h_free_crq,
	[H_VIO_SIGNAL/4]		= h_vio_signal,
	[H_SEND_CRQ/4]			= h_send_crq,
	[H_PUTRTCE/4]			= h_putrtce,
	[H_COPY_RDMA/4]			= h_copy_rdma,
	[H_REGISTER_LOGICAL_LAN/4]	= h_register_logical_lan,
	[H_FREE_LOGICAL_LAN/4]		= h_free_logical_lan,
	[H_ADD_LOGICAL_LAN_BUFFER/4]	= h_add_logical_lan_buffer,
	[H_SEND_LOGICAL_LAN/4]		= h_send_logical_lan,
	[H_BULK_REMOVE/4]		= h_bulk_remove,
	[H_WRITE_RDMA/4]		= h_write_rdma,
	[H_READ_RDMA/4]			= h_read_rdma,
	[H_MULTICAST_CTRL/4]		= h_multicast_ctrl,
	[H_SET_XDABR/4]			= h_set_xdabr,
	[H_STUFF_TCE/4]			= h_stuff_tce,
	[H_PUT_TCE_INDIRECT/4]		= h_put_tce_indirect,
	[H_PUT_RTCE_INDERECT/4]		= h_put_rtce_inderect,
	[H_MASS_MAP_TCE/4]		= h_mass_map_tce,
	[H_ALRDMA/4]			= h_alrdma,
	[H_CHANGE_LOGICAL_LAN_MAC/4]	= h_change_logical_lan_mac,
	[H_VTERM_PARTNER_INFO/4]	= h_vterm_partner_info,
	[H_REGISTER_VTERM/4]		= h_register_vterm,
	[H_FREE_VTERM/4]		= h_free_vterm,
	[H_GRANT_LOGICAL/4]		= h_grant_logical,
	[H_RESCIND_LOGICAL/4]		= h_rescind_logical,
	[H_ACCEPT_LOGICAL/4]		= h_accept_logical,
	[H_RETURN_LOGICAL/4]		= h_return_logical,
	[H_FREE_LOGICAL_LAN_BUFFER/4]	= h_free_logical_lan_buffer,

	[H_HCA_RESV_BEGIN/4] = h_hca_resv,
	[H_HCA_RESV_END/4] = h_hca_resv,

//	[(H_HCA_RESV_BEGIN/4) ... (H_HCA_RESV_END/4)] = h_hca_resv,
	/* *INDENT-ON* */
};

const uval hcall_vec_len = sizeof (hcall_vec);

/*
 * 0x6000-series hcall table
 */
const hcall_vec_t hcall_vec6000[] = {
	/* *INDENT-OFF* */
	[HEID2X(H_YIELD)]		= h_yield,
	[HEID2X(H_CREATE_MSGQ)]		= h_create_msgq,
	[HEID2X(H_SEND_ASYNC)]		= h_send_async,
	[HEID2X(H_CREATE_PARTITION)]	= h_create_partition,
	[HEID2X(H_START)]		= h_start,
	[HEID2X(H_GET_LPID)]		= h_get_lpid,
	[HEID2X(H_SET_SCHED_PARAMS)]	= h_set_sched_params,
	[HEID2X(H_RESOURCE_TRANSFER)]	= h_resource_transfer,
	[HEID2X(H_MULTI_PAGE)]		= h_multi_page,
	[HEID2X(H_VM_MAP)]		= h_vm_map,
	[HEID2X(H_DESTROY_PARTITION)]	= h_destroy_partition,
	[HEID2X(H_RTAS)]		= h_rtas,
	[HEID2X(H_VIO_CTL)]		= h_vio_ctl,
	[HEID2X(H_DEBUG)]		= h_debug,
	[HEID2X(H_LPAR_INFO)]		= h_lpar_info,
	[HEID2X(H_MEM_DEFINE)]		= h_mem_define,
	[HEID2X(H_EIC_CONFIG)]		= h_eic_config,
	[HEID2X(H_HTAB)]		= h_htab,

#ifdef SPC_HCALLS
	/* FIXME: need a better way to do this, perhpas another set of
	 * vectors, but that could be overkill */
	SPC_HCALLS,
#endif

	/* hidden */
	[HEID2X(H_PCI_CONFIG)]		= h_pci_config,
	[HEID2X(H_PCI_CONFIG_READ)]	= h_pci_config_read,
	[HEID2X(H_PCI_CONFIG_WRITE)]	= h_pci_config_write,
	[HEID2X(H_GET_XIVE)]		= h_get_xive,
	[HEID2X(H_SET_XIVE)]		= h_set_xive,
	[HEID2X(H_INTERRUPT)]		= h_interrupt,
	[HEID2X(H_THREAD_CONTROL)]	= h_thread_control,
	/* *INDENT-ON* */
};

const uval hcall_vec6000_len = sizeof (hcall_vec6000);
