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
#include <hba.h>
#include <lpar.h>

static const char nothing[] = "nothing here";
uval
hba_config_read(uval lpid, uval addr, uval buid, uval size, uval *val)
{
	(void)lpid;
	(void)addr;
	(void)buid;
	(void)size;
	(void)val;
	assert(0, nothing);
	return 0;
}

uval
hba_config_write(uval lpid, uval addr, uval buid, uval size, uval val)
{
	(void)lpid;
	(void)addr;
	(void)buid;
	(void)size;
	(void)val;
	assert(0, nothing);
	return 0;
}

extern void __hba_init(void);
sval
hba_init(uval ofd)
{
	(void)ofd;
	__hba_init();
	return H_Success;
}
