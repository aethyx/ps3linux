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
#ifndef _HYPE_CALLS_H
#define _HYPE_CALLS_H

#include <hypervisor.h>
#include <os.h>

extern const hcall_vec_t hcall_vec[];
extern const uval hcall_vec_len;
extern const hcall_vec_t hcall_vec6000[];
extern const uval hcall_vec6000_len;

extern sval h_invalid(struct cpu_thread *thread);
extern sval h_remove(struct cpu_thread *thread, uval flags,
		     uval ptex, uval avpn);
extern sval h_enter(struct cpu_thread *thread, uval flags, uval ptex,
		    uval vsidWord, uval rpnWord);
extern sval h_read(struct cpu_thread *thread, uval flags, uval ptex);
extern sval h_clear_mod(struct cpu_thread *thread, uval flags, uval ptex);
extern sval h_clear_ref(struct cpu_thread *thread, uval flags, uval ptex);
extern sval h_protect(struct cpu_thread *thread, uval flags,
		      uval ptex, uval avpn);
extern sval h_get_tce(struct cpu_thread *thread, uval32 liobn, uval ioba);
extern sval h_put_tce(struct cpu_thread *thread, uval32 liobn, uval ioba,
		      uval64 tce_dword);
extern sval h_stuff_tce(struct cpu_thread *thread, uval32 liobn, uval ioba,
			uval64 tce_dword, uval count);
extern sval h_vm_map(struct cpu_thread *thread, uval flags, uval pvpn,
		     uval ptel);
extern sval h_real_to_logical(struct cpu_thread *thread, uval raddr);
extern sval h_set_sprg0(struct cpu_thread *thread);
extern sval h_set_dabr(struct cpu_thread *thread);
extern sval h_page_init(struct cpu_thread *thread, uval flags,
			uval destination, uval source);
extern sval h_logical_ci_load(struct cpu_thread *thread, uval size, uval addr);
extern sval h_logical_ci_store(struct cpu_thread *thread, uval size, uval addr,
			       uval value);
extern sval h_logical_cache_load(struct cpu_thread *thread, uval size,
				 uval addr);
extern sval h_logical_cache_store(struct cpu_thread *thread, uval size,
				  uval addr, uval value);
extern sval h_logical_icbi(struct cpu_thread *thread, uval addr);
extern sval h_logical_dcbf(struct cpu_thread *thread, uval addr);
extern sval h_hypervisor_data(struct cpu_thread *thread, uval control);

/* IPC */
extern sval h_create_msgq(struct cpu_thread *thread, uval lbase,
			  uval size, uval flags);
extern sval h_send_async(struct cpu_thread *thread, uval dest,
			 uval arg1, uval arg2, uval arg3, uval arg4);

/* VTTY */
extern sval h_get_term_char(struct cpu_thread *thread, uval channel);
extern sval h_put_term_char(struct cpu_thread *thread, uval channel, uval c,
			    uval ch1, uval ch2,  uval ch3, uval ch4);

/* Interrupts */
extern sval h_eoi(struct cpu_thread *thread, uval xirr);
extern sval h_cppr(struct cpu_thread *thread, uval cppr);
extern sval h_ipi(struct cpu_thread *thread, uval server_num, uval mfrr);
extern sval h_ipoll(struct cpu_thread *thread, uval server_num);

/* weakened interfaces that can be interposed */
extern sval __h_ipoll(struct cpu_thread *thread, uval server_num);
extern sval __h_ipi(struct cpu_thread *thread, uval server_num, uval mfrr);
extern sval h_xirr(struct cpu_thread *thread);

/* Scheduling */
extern sval h_yield(struct cpu_thread *thread, uval lpid);
extern sval h_create_partition(struct cpu_thread *thread, uval l_addr,
			       uval l_size, uval pinfo_offset);
extern sval h_destroy_partition(struct cpu_thread *thread, uval lpid);
extern sval h_start(struct cpu_thread *thread, int lpid, uval pc,
		    uval reg2, uval reg3, uval reg4,
		    uval reg5, uval reg6, uval reg7);
extern sval h_set_exception_info(struct cpu_thread *thread, uval cpu,
				 uval flags, uval srrloc, uval exvec);
extern sval h_get_lpid(struct cpu_thread *thread);
extern sval h_set_sched_params(struct cpu_thread *thread, uval lpid,
			       uval cpu, uval required, uval desired);
/* Memory Management */
extern sval h_resource_transfer(struct cpu_thread *thread, uval type,
				uval addr_hi, uval addr_lo,
				uval size, uval lpid);
extern sval h_multi_page(struct cpu_thread *thread, uval s1, uval s2, uval s3);

/* ISTAR only. */
extern sval h_set_asr(struct cpu_thread *thread, uval value);
extern sval h_asr_on(struct cpu_thread *thread);
extern sval h_asr_off(struct cpu_thread *thread);

extern sval h_confer(struct cpu_thread *thread);
extern sval h_prod(struct cpu_thread *thread);
extern sval h_get_ppp(struct cpu_thread *thread);
extern sval h_set_ppp(struct cpu_thread *thread);
extern sval h_purr(struct cpu_thread *thread);
extern sval h_pic(struct cpu_thread *thread);
extern sval h_vio_ctl(struct cpu_thread *thread, uval cmd,
		      uval type, uval drange);
extern sval h_vio_signal(struct cpu_thread *thread, uval uaddr, uval mode);
extern sval h_reg_crq(struct cpu_thread *thread, uval uaddr,
		      uval queue, uval len);
extern sval h_free_crq(struct cpu_thread *thread, uval u_addr);
extern sval h_send_crq(struct cpu_thread *thread, uval u_addr,
		       uval64 msg_hi, uval64 msg_lo);
extern sval h_putrtce(struct cpu_thread *thread);
extern sval h_copy_rdma(struct cpu_thread *thread, uval length,
			uval sliobn, uval sioba, uval dliobn, uval dioba);
extern sval h_register_logical_lan(struct cpu_thread *thread, uval uaddr,
				   uval bl, uval64 rq, uval fl, uval64 mac);
extern sval h_free_logical_lan(struct cpu_thread *thread, uval uaddr);
extern sval h_add_logical_lan_buffer(struct cpu_thread *thread,
				     uval uaddr, uval64 bufd);
extern sval h_send_logical_lan(struct cpu_thread *thread, uval uaddr,
			       uval64 bd1, uval64 bd2, uval64 bd3,
			       uval64 bd4, uval64 bd5, uval64 bd6, uval token);
extern sval h_bulk_remove(struct cpu_thread *thread);
extern sval h_write_rdma(struct cpu_thread *thread);
extern sval h_read_rdma(struct cpu_thread *thread);
extern sval h_multicast_ctrl(struct cpu_thread *thread, uval uaddr,
			     uval64 flags, uval64 mmac);
extern sval h_set_xdabr(struct cpu_thread *thread);
extern sval h_put_tce_indirect(struct cpu_thread *thread);
extern sval h_put_rtce_inderect(struct cpu_thread *thread);
extern sval h_mass_map_tce(struct cpu_thread *thread);
extern sval h_alrdma(struct cpu_thread *thread);
extern sval h_change_logical_lan_mac(struct cpu_thread *thread,
				     uval uaddr, uval64 mac);
extern sval h_vterm_partner_info(struct cpu_thread *thread, uval uaddr,
				 uval plpid, uval puaddr, uval lba);
extern sval h_register_vterm(struct cpu_thread *thread, uval uaddr,
			     uval plpid, uval puaddr);
extern sval h_free_vterm(struct cpu_thread *thread, uval ua);
extern sval h_grant_logical(struct cpu_thread *thread, uval flags,
			    uval logical_hi, uval logical_lo, uval length,
			    uval unit_address);
extern sval h_accept_logical(struct cpu_thread *thread, uval cookie);

extern sval h_rescind_logical(struct cpu_thread *thread, uval flags,
			      uval cookie);
extern sval h_return_logical(struct cpu_thread *thread);
extern sval h_free_logical_lan_buffer(struct cpu_thread *thread,
				      uval uaddr, uval size);
extern sval h_hca_resv(struct cpu_thread *thread);

/*
 * "Hidden" hcalls()
 */

extern sval h_get_xive(struct cpu_thread *thread, uval32 intr);
extern sval h_set_xive(struct cpu_thread *thread, uval32 intr,
		       uval32 serv, uval32 prio);
extern sval h_interrupt(struct cpu_thread *thread, uval intr, uval set);
extern sval h_pci_config_read(struct cpu_thread *thread, uval caddr, uval buid,
			      uval size);
extern sval h_pci_config_write(struct cpu_thread *thread, uval caddr,
			       uval buid, uval size, uval val);
extern sval h_pci_config(struct cpu_thread *thread, uval command, uval buid,
			 uval arg1, uval arg2, uval arg3);

extern sval h_eic_config(struct cpu_thread *thread, uval command,
			 uval arg1, uval arg2, uval arg3, uval arg4);

extern sval h_mem_define(struct cpu_thread *thread, uval type, uval addr,
			 uval size);

extern sval h_debug(struct cpu_thread *thread, uval cmd, uval arg1, uval arg2,
		    uval arg3, uval arg4);
extern sval h_lpar_info(struct cpu_thread *thread, uval cmd, uval arg1,
			uval arg2, uval arg3, uval arg4);

#endif /* ! _HYPE_CALLS_H */
