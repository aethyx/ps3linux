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
 * Compiler specific defines.
 *
 */
#ifndef _COMPILER_H
#define _COMPILER_H

#if (__GNUC__ <= 2) && (__GNUC_MINOR__ < 96)
#define __builtin_expect(x, expected_value) (x)
#endif

/*
 * Provide hints to the code generator about which branch of an
 * if-then-else statement is most likely.
 */
#define	likely(cond)	__builtin_expect(!!(cond), 1)
#define	unlikely(cond)	__builtin_expect(!!(cond), 0)

#endif /* _COMPILER_H */
