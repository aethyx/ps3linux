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

#include <config.h>
#include <lib.h>
#include <hypervisor.h>
#include <stack.h>
#include <thread_control_area.h>
#include <boot.h>
#include <atomic.h>

uval
boot_smp_watch(struct boot_cpu_info *ci)
{
	hprintf("000 0x%lx\n", mftb());

	while (!cas_uval(&ci->bci_spin, 1, 2)) ;
	hprintf("%s: CPU 0x%lx is alive\n", __func__, ci->bci_cpuno);

	cas_uval(&ci->bci_spin, 2, 3);
	while (!cas_uval(&ci->bci_spin, 5, 0)) ;

	hprintf("000 0x%lx\n", mftb());

	hprintf("%s: off to find more CPUs\n", __func__);

	return 1;
}

void __attribute__ ((noreturn))
boot_smp_start(struct boot_cpu_info *ci)
{
	struct thread_control_area *tca;
	uval pir;

	hprintf("XXX 0x%lx\n", mftb());
	cas_uval(&ci->bci_spin, 0, 1);

	while (!cas_uval(&ci->bci_spin, 3, 4)) ;
	hprintf("%s: initing  CPU core: 0x%lx\n", __func__, ci->bci_cpuno);
	cpu_core_init(ci->bci_ofd, ci->bci_cpuno, ci->bci_cpuseq);

	tca = tca_from_stack(ci->bci_stack);
	tca_init(tca, &hca, ci->bci_stack, 0);

	/* enter into global table for system reset */
	pir = mfpir();
	hprintf("%s: registering PIR: 0x%lx\n", __func__, pir);
	tca_table_enter(pir, tca);
	cca_table_enter(ci->bci_cpuseq, tca->cca);

	hprintf("%s: cpu 0x%lx finished init\n", __func__, ci->bci_cpuno);
	
	cas_uval(&ci->bci_spin, 4, 5);
	hprintf("XXX 0x%lx\n", mftb());

	/* wait for controller to get started */
	struct cpu_control_area __volatile__ *cca = tca->cca;

	while (cca->active_cpu == NULL) ;

	cpu_idle();
}
