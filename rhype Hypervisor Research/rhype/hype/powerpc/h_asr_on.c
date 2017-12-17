
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
#include <h_proto.h>

sval
h_asr_on(struct cpu_thread *thread __attribute__ ((unused)))
{
#if HAS_ASR
	/* Generic powerpc architecture does not support valid
	 * bit. SStar has a lot more than just valid bit.
	 */
	thread->reg_asr |= 0x1;	/* bit 63 is the valid bit */
	return (H_Success);
#else
	return (H_Function);
#endif
}
