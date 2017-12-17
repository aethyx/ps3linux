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

#ifndef _CRQ_H
#define _CRQ_H

#include <types.h>
#include <cpu.h>
#include <tce.h>

/*
 * the maxiumum number of global CRQs is the maximum slot 
 * number that is available
 */
#define MAX_GLOBAL_CRQ	(8 * sizeof (uval64))

/*
 * Minimum buffer size of a crq required when registering.
 */
#define CRQ_MIN_BUFFER_SIZE     4096
#define CRQ_DMA_WINDOW_SIZE     (8 * 1024 * 1024)

extern sval crq_sys_init(void);
extern sval crq_os_release_all(struct os *os);
extern sval crq_try_copy(struct cpu_thread *thread, uval32 sliobn, uval sioba,
			 uval32 dliobn, uval dioba, uval len);
extern sval crq_reg(struct cpu_thread *thread, uval uaddr,
		    uval queue, uval len);
extern sval crq_free_crq(struct os *os, uval uaddr);
extern sval crq_send(struct os *os, uval uaddr, uval8 *msg);


#endif /* _CRQ_H */
