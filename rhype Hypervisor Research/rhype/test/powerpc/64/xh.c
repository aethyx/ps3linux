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

#include <test.h>
#include <xh.h>
#include <xirr.h>
#include <mmu.h>

static uval
xh_assert(uval ex, uval *regs)
{
	assert(0,
	       "Exception [0x%lx]: should never happen: 0x%lx, 0x%lx\n",
	       ex, regs[XHR_SRR0], regs[XHR_SRR1]);
	return 0;
}

extern char _end[];

static uval xh_handle_tlb_miss(uval ex, uval *regs)
{
	uval vaddr = (uval)&_text_start;
	uval size = (uval)_end - vaddr;
	uval eaddr = 0;
	uval raddr = 0;
	uval offset;
	uval8 l = 1;
	uval8 ls = SELECT_LG;
	struct partition_info *pi = &pinfo[1];
	uval lp = get_log_pgsize(pi, l, ls);
	uval pgsize = 1 << lp;

	if (ex == 0x400) {
		/* ISI */
		eaddr = regs[XHR_SRR0];
	} else {
		/* DSI */
		eaddr = regs[XHR_DAR];
	}

#ifdef DEBUG
	hprintf("xh_handle_tlb_miss(): "
		"eaddr 0x%lx srr0 0x%lx srr1 0x%lx\n",
		eaddr, regs[XHR_SRR0], regs[XHR_SRR1]);
#endif

	for (offset = 0; offset < size; offset += pgsize) {
		if ((eaddr >= (vaddr + offset)) &&
		   (eaddr < (vaddr + offset + pgsize)))
		{
			return tlb_enter(pi, vaddr + offset,
				  raddr + offset,
				  PAGE_WIMG_M, l, ls);
		}
	}

	return 1;
}

static uval
xh_dsi(uval ex, uval *regs)
{
	uval ret;

	ret = xh_handle_tlb_miss(ex, regs);
	assert(ret == 0, "xh_dsi failed\n");

	return 0;
}

static uval
xh_isi(uval ex, uval *regs)
{
	uval ret;

	ret = xh_handle_tlb_miss(ex, regs);
	assert(ret == 0, "xh_isi failed\n");

	return 0;
}

#if JX_REPAIR
/* are we gonna do something with these return codes */
static uval
xh_external(uval ex __attribute__ ((unused)),
	    uval *regs __attribute__ ((unused)))
{
	uval ret[1];
	uval rc;
	xirr_t xirr;

	rc = hcall_xirr(ret);
	assert((rc == H_Success),
	       "hcall_xirr failed\n");

	xirr = ret[0];

#ifdef DEBUG
	hprintf("%s: external interupt with xirr: 0x%x\n", __func__, xirr);
#endif
	if (xirr_handle(xh_data, xirr, 0, NULL) >= 0) {
#ifdef DEBUG
	hprintf("%s: calling hcall EOI: 0x%x\n", __func__, xirr);
#endif
		rc = hcall_eoi(NULL, xirr);
		assert((rc == H_Success),
		       "hcall_eoi failed\n");
		return 1;
	}
	assert(0, "%s: xirr_handle failed\n", __func__);
	return 0;
}
#endif

xh_func_t xh_table[] = {
	[XH_SYSRESET]	= xh_assert,
	[XH_MACHCHECK]	= xh_assert,
	[XH_DSI]	= xh_dsi,
	[XH_DATA_SLB]	= xh_assert,
	[XH_ISI]	= xh_isi,
	[XH_INST_SLB]	= xh_assert,
	[XH_INTERRUPT]	= aipc_handler,
	[XH_ALIGNMENT]	= xh_assert,
	[XH_PROGRAM]	= xh_assert,
	[XH_FLOAT]	= xh_assert,
	[XH_DEC]	= NULL,
	[XH_HDEC]	= xh_assert,
	[XH_SYSCALL]	= xh_assert,
	[XH_TRACE]	= NULL,
	[XH_FP]		= xh_assert,
	[XH_SONY]	= xh_assert,
	[XH_GENERIC]	= xh_assert,
	[XH_AIPC]	= xh_assert
};
