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
 *
 * Routines to read and write bytes in memory mapped space.
 *
 */

#include <config.h>

/*
 * TODO: we probably need a better to handle architecture specific files.
 * for now ifndef works.
 */ 
#ifndef CPU_4xx

#include <asm.h>

# In order to access device registers (and not just look at the cache)
# the address space must be accessed with caching inhibited.  Use BAT 0
# for this purpose.  Just in case it was defined for something else,
# save its contents in r8 and r9, and then restore them before exiting.
#
# void ioOutUchar(address, value);
#
# value = ioInUchar(address);
#
# WARNING:  NOT SMP SAFE, Interrupts may also cause problems.

	C_TEXT_ENTRY(ioOutUchar)
        mfspr   8, 536          # save current bat contents
        mfspr   9, 537          # mfdbatl 0, 9

        li      6, 0            # disable the bat while writing it
        mtdbatu 0, 6
        isync
        lis     6, 0xfffc       # load upper part of address
        and     6, 3, 6
        rldicl  6, 6, 0, 32     # clear upper part
        ori     7, 6, 0x002a    # WIMG = 0101, PP=2 (r/w)
        mtdbatl 0, 7
        ori     7, 6, 0x0002    # 128KB block length, Vs=1, Vp=0
        mtdbatu 0, 7

        mfmsr   6
        ori     7, 6, 0x0010    # turn on data relocation only
        MTMSR   7
        isync

        stbx    4, 0, 3         # write the byte we want
        eieio

        MTMSR   6               # restore the msr
        isync

        li      6, 0            # restore bat contents
        mtdbatu 0, 6
        isync
        mtdbatl 0, 9
        mtdbatu 0, 8
        isync

        blr

	C_TEXT_ENTRY(ioInUchar)
        mfspr   8, 536          # save current bat contents
        mfspr   9, 537          # mfdbatl 0,9

        li      6, 0            # disable the bat while writing it
        mtdbatu 0, 6
        isync
        lis     6, 0xfffc       # load upper part of address
        and     6, 3, 6
        rldicl  6, 6, 0, 32     # clear upper part
        ori     7, 6, 0x002a    # WIMG = 0101, PP=2 (r/w)
        mtdbatl 0, 7
        ori     7, 6, 0x0002    # 128KB block length, Vs=1, Vp=0
        mtdbatu 0, 7

        mfmsr   6
        ori     7, 6, 0x0010    # turn on data relocation only
        MTMSR   7
        isync

        lbzx    3, 0, 3         # read the byte we want
        eieio

        MTMSR   6               # restore the msr
        isync

        li      6, 0            # restore bat contents
        mtdbatu 0, 6
        isync
        mtdbatl 0, 9
        mtdbatu 0, 8
        isync

        blr

#endif /* ! CPU_4xx */
