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
#include <types.h>
#include <openfirmware.h>
#include <asm.h>


typedef sval32 (*prom_call_t)(void *, uval);

sval32
prom_call(void *arg, uval base, uval func, uval msr __attribute__ ((unused)))
{
	prom_call_t f = (prom_call_t)func;
	uval srr0 = mfsrr0();
	uval srr1 = mfsrr1();
	sval32 ret = f(arg, base);

	mtsrr0(srr0);
	mtsrr1(srr1);

	return ret;

	
}
