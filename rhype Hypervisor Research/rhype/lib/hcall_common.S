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
 * WARNING: the 3rd parameter is sometimes "mangled" using the
 * "##" CPP operation.  Some buggy CPPs will preserv the whitespace
 * after the ',' so we break style for this reason.
 */
HCALL(5, 0,hcall_vm_map, H_VM_MAP)
HCALL(5, 2,hcall_remove, H_REMOVE)
HCALL(5, 1,hcall_clear_mod, H_CLEAR_MOD)
HCALL(5, 1,hcall_clear_ref, H_CLEAR_REF)
HCALL(5, 0,hcall_protect, H_PROTECT)
HCALL(5, 2,hcall_set_exception_info, H_SET_EXCEPTION_INFO)
HCALL(5, 0,hcall_start, H_START)
HCALL(1, 0,hcall_yield, H_YIELD)
HCALL(3, 1,hcall_create_msgq, H_CREATE_MSGQ)
HCALL(5, 0,hcall_send_async, H_SEND_ASYNC)
HCALL(3, 1,hcall_create_partition, H_CREATE_PARTITION)
HCALL(5, 0,hcall_destroy_partition, H_DESTROY_PARTITION)
HCALL(4, 1,hcall_resource_transfer, H_RESOURCE_TRANSFER)
HCALL(1, 1,hcall_get_lpid, H_GET_LPID)
HCALL(5, 3,hcall_set_sched_params, H_SET_SCHED_PARAMS)
HCALL(1, 0,hcall_eoi, H_EOI)
HCALL(5, 1,hcall_cppr, H_CPPR)
HCALL(5, 2,hcall_ipi, H_IPI)
HCALL(5, 1,hcall_ipoll, H_IPOLL)
HCALL(5, 1,hcall_xirr, H_XIRR)
HCALL(2, 0,hcall_interrupt, H_INTERRUPT)
HCALL(5, 1,hcall_logical_ci_load_64, H_LOGICAL_CI_LOAD)
HCALL(5, 0,hcall_logical_ci_store_64, H_LOGICAL_CI_STORE)
HCALL(5, 1,hcall_logical_cache_load_64, H_LOGICAL_CACHE_LOAD)
HCALL(5, 0,hcall_logical_cache_store_64, H_LOGICAL_CACHE_STORE)
HCALL(5, 0,hcall_logical_icbi, H_LOGICAL_ICBI)
HCALL(5, 0,hcall_logical_dcbf, H_LOGICAL_DCBF)
HCALL(5, 1,hcall_set_dabr, H_SET_DABR)
HCALL(5, 1,hcall_real_to_logical, H_REAL_TO_LOGICAL)
HCALL(5, 0,hcall_multi_page, H_MULTI_PAGE)
HCALL(5, 1,hcall_pci_config_read, H_PCI_CONFIG_READ)
HCALL(5, 0,hcall_pci_config_write, H_PCI_CONFIG_WRITE)
HCALL(5, 5,hcall_pci_config, H_PCI_CONFIG)
HCALL(5, 5,hcall_eic_config, H_EIC_CONFIG)
HCALL(3, 1,hcall_mem_define, H_MEM_DEFINE)
HCALL(5, 1,hcall_grant_logical, H_GRANT_LOGICAL)
HCALL(1, 1,hcall_accept_logical, H_ACCEPT_LOGICAL)
HCALL(0, 2,hcall_rescind_logical, H_RESCIND_LOGICAL)
HCALL(3, 0,hcall_register_vterm, H_REGISTER_VTERM)
HCALL(4, 0,hcall_vterm_partner_info, H_VTERM_PARTNER_INFO)
HCALL(1, 0,hcall_free_vterm, H_FREE_VTERM)
HCALL(5, 5,hcall_debug, H_DEBUG)
HCALL(5, 5,hcall_lpar_info, H_LPAR_INFO)
HCALL(3, 3,hcall_vio_ctl, H_VIO_CTL)
