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
#ifndef _EIC_H
#define _EIC_H

#include <config.h>
#include <types.h>
#include <cpu_thread.h>
#include <eic.h>
#include <xirr.h>

extern lock_t eic_lock;
extern struct cpu_thread *eic_owner;

extern struct cpu_thread *eic_exception(struct cpu_thread *thread);
extern sval eic_set_line(uval lpid, uval cpu, uval thr,
			 uval hw_src, uval *isrc);

extern sval eic_config(uval arg1, uval arg2, uval arg3, uval arg4);

/* identify the default handler thread */
extern struct cpu_thread *eic_default_thread(void);

/* This function is called when there is an interrupt that doesn't have
 * and handler thread associated with it. */
xirr_func_t *eic_default_intr_handler;

#endif /* _EIC_H */
