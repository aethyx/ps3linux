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
#include <hype.h>
#include <lpar.h>
#include <hcall.h>
#include <os.h>
#include <pmm.h>
#include <vm.h>
#include <h_proto.h>

#define	RPA_INDEX(i)	((i) / 4)
#define	HYPE_INDEX(i)	(((i) - HYPE_HCALL_BASE) / 4)

/*
 * hcall table for x86
 */
static const hcall_vec_t rpa_hcall_vec[(RPA_HCALL_END / 4) + 1] = {
	/* *INDENT-OFF* */
	[RPA_INDEX(H_GET_TERM_CHAR)]	      = {1, h_get_term_char},
	[RPA_INDEX(H_PUT_TERM_CHAR)]	      = {6, h_put_term_char},
	[RPA_INDEX(H_EOI)]		      = {1, h_eoi},
	[RPA_INDEX(H_XIRR)]		      = {0, h_xirr},
	[RPA_INDEX(H_GET_TCE)]		      = {2, h_get_tce},
	[RPA_INDEX(H_PUT_TCE)]		      = {4, h_put_tce},
	[RPA_INDEX(H_VIO_SIGNAL)]	      = {2, h_vio_signal},
	[RPA_INDEX(H_REGISTER_LOGICAL_LAN)]   = {7, h_register_logical_lan},
	[RPA_INDEX(H_FREE_LOGICAL_LAN)]	      = {1, h_free_logical_lan},
	[RPA_INDEX(H_ADD_LOGICAL_LAN_BUFFER)] = {3, h_add_logical_lan_buffer},
	[RPA_INDEX(H_SEND_LOGICAL_LAN)]	      = {14, h_send_logical_lan},
	[RPA_INDEX(H_MULTICAST_CTRL)]	      = {5, h_multicast_ctrl},
	[RPA_INDEX(H_CHANGE_LOGICAL_LAN_MAC)] = {3, h_change_logical_lan_mac},
	[RPA_INDEX(H_FREE_LOGICAL_LAN_BUFFER)] =
		{2, h_free_logical_lan_buffer},
	[RPA_INDEX(H_REG_CRQ)]                = {4, h_reg_crq},
	[RPA_INDEX(H_SEND_CRQ)]               = {5, h_send_crq},
	[RPA_INDEX(H_FREE_CRQ)]               = {1, h_free_crq},
	[RPA_INDEX(H_COPY_RDMA)]              = {5, h_copy_rdma},
	[RPA_INDEX(H_REGISTER_VTERM)]         = {3, h_register_vterm},
	[RPA_INDEX(H_FREE_VTERM)]             = {1, h_free_vterm},
	[RPA_INDEX(H_GRANT_LOGICAL)]	      = {5, h_grant_logical }, 
	[RPA_INDEX(H_RESCIND_LOGICAL)]	      = {0, h_rescind_logical },
	[RPA_INDEX(H_ACCEPT_LOGICAL)]	      = {1, h_accept_logical },
	/* *INDENT-ON* */
};

static const hcall_vec_t hype_hcall_vec[1 + HYPE_INDEX(HYPE_HCALL_END)] = {
	/* *INDENT-OFF* */
	[HYPE_INDEX(H_YIELD)]			= {1, h_yield},
	[HYPE_INDEX(H_SEND_ASYNC)]		= {5, h_send_async},
	[HYPE_INDEX(H_GET_LPID)]		= {1, h_get_lpid},
	[HYPE_INDEX(H_CREATE_MSGQ)]		= {3, h_create_msgq},
	[HYPE_INDEX(H_CREATE_PARTITION)]	= {3, h_create_partition},
	[HYPE_INDEX(H_DESTROY_PARTITION)]	= {5, h_destroy_partition},
	[HYPE_INDEX(H_START)]			= {5, h_start},
	[HYPE_INDEX(H_DT_ENTRY)]		= {4, h_dt_entry},
	[HYPE_INDEX(H_PAGE_DIR)]		= {2, h_page_dir},
	[HYPE_INDEX(H_SET_MBOX)]		= {1, h_set_mbox},
	[HYPE_INDEX(H_FLUSH_TLB)]		= {2, h_flush_tlb},
	[HYPE_INDEX(H_GET_PFAULT_ADDR)]		= {0, h_get_pfault_addr},
	[HYPE_INDEX(H_SYS_STACK)]		= {2, h_sys_stack},
	[HYPE_INDEX(H_SET_SCHED_PARAMS)]	= {5, h_set_sched_params},
	[HYPE_INDEX(H_GET_PTE)]	        	= {2, h_get_pte},
	[HYPE_INDEX(H_DEBUG)]			= {5, h_debug},
	[HYPE_INDEX(H_LPAR_INFO)]		= {5, h_lpar_info},
	[HYPE_INDEX(H_INTERRUPT)]		= {2, h_interrupt},
	[HYPE_INDEX(H_DR)]			= {9, h_dr},
	[HYPE_INDEX(H_VIO_CTL)]			= {3, h_vio_ctl},
	[HYPE_INDEX(H_RESOURCE_TRANSFER)]	= {6, h_resource_transfer},
	/* *INDENT-ON* */
};

uval
get_thread_addr(void)
{
	struct tss *cur_tss = get_cur_tss();

	assert(cur_tss != NULL, "no current TSS");
	return cur_tss->threadp;
}

/*
 * Hypervisor call entry point.
 * Only code running at PL 2 can make hcalls.
 */
void
hcall(struct cpu_thread *thread)
{
	sval index = thread->tss.gprs.regs.eax;
	const hcall_vec_t *call = NULL;

	if (index >= 0 && index <= RPA_HCALL_END) {
		call = &rpa_hcall_vec[RPA_INDEX(index)];
	} else if (index >= HYPE_HCALL_BASE && index <= HYPE_HCALL_END) {
		call = &hype_hcall_vec[HYPE_INDEX(index)];
	}

	/*
	 * There are two cases. One where all the arguments fit into
	 * the registers. This is the simple case and we just call the
	 * corresponding hcall handler. If more than 6 arguments are
	 * needed we copy it from the guest stack. This requires a lot
	 * more care. We currently copy up to 16 arguments.
	 */
	if (likely(call != NULL && call->hcall != NULL)) {
		if (likely(call->ins < 6)) {
			/* all arguments fit into the registers */
			thread->tss.gprs.regs.eax =
				call->hcall(thread,
					    thread->tss.gprs.regs.ecx,
					    thread->tss.gprs.regs.edx,
					    thread->tss.gprs.regs.ebx,
					    thread->tss.gprs.regs.esi,
					    thread->tss.gprs.regs.edi);
		} else {
			uval args[16];

			if (!get_lpar(thread, args,
				      thread->tss.gprs.regs.ebp + 12,
				      sizeof (args))) {
				thread->tss.gprs.regs.eax = H_Parameter;
				return;
			}

			thread->tss.gprs.regs.eax =
				call->hcall(thread,
					    args[0], args[1], args[2], args[3],
					    args[4], args[5], args[6], args[7],
					    args[8], args[9], args[10],
					    args[11], args[12], args[13],
					    args[14], args[15]);
		}
	} else {
		thread->tss.gprs.regs.eax = H_Function;
	}
}
