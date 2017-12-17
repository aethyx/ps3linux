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
#include <rh.h>
#include <hcall.h>
#include <lpar.h>

sval
rh_read_pci_config(uval nargs, uval nrets, uval argp[], uval retp[],
		   uval ba __attribute__ ((unused)))
{
	if (nargs == 2) {
		if (nrets == 2) {
			uval conf_addr = argp[0];
			uval sz = argp[1];
			uval rets[1];
			sval rc;

			rc = hcall_pci_config_read(rets, conf_addr, 0, sz);
			if (rc == H_Success) {
				retp[0] = RTAS_SUCCEED;
				retp[1] = rets[0];
				return RTAS_SUCCEED;
			}
			retp[0] = RTAS_HW_ERROR;
			return RTAS_HW_ERROR;
		}
	}
	retp[0] = RTAS_PARAM_ERROR;
	return RTAS_PARAM_ERROR;
}

sval
rh_ibm_read_pci_config(uval nargs, uval nrets, uval argp[], uval retp[],
		       uval ba __attribute__ ((unused)))
{
	if (nargs == 4) {
		if (nrets == 2) {
			uval conf_addr = argp[0];
			uval buid;
			uval sz = argp[3];
			uval rets[1];
			sval rc;

			buid = argp[1] << ((sizeof (uval) - 4) * 8);
			buid |= argp[2];

			rc = hcall_pci_config_read(rets, conf_addr, buid, sz);
			if (rc == H_Success) {
				retp[0] = RTAS_SUCCEED;
				retp[1] = rets[0];
				return RTAS_SUCCEED;
			}
			retp[0] = RTAS_HW_ERROR;
			return RTAS_HW_ERROR;
		}
	}
	retp[0] = RTAS_PARAM_ERROR;
	return RTAS_PARAM_ERROR;
}
sval
rh_write_pci_config(uval nargs, uval nrets, uval argp[], uval retp[],
		    uval ba __attribute__ ((unused)))
{
	if (nargs == 3) {
		if (nrets == 1) {
			uval conf_addr = argp[0];
			uval sz = argp[1];
			uval val = argp[2];
			sval rc;

			rc = hcall_pci_config_write(NULL, conf_addr, 0,
						    sz, val);
			if (rc == H_Success) {
				retp[0] = RTAS_SUCCEED;
				return RTAS_SUCCEED;
			}
			retp[0] = RTAS_HW_ERROR;
			return RTAS_HW_ERROR;
		}
	}
	retp[0] = RTAS_PARAM_ERROR;
	return RTAS_PARAM_ERROR;
}
sval
rh_ibm_write_pci_config(uval nargs, uval nrets, uval argp[], uval retp[],
			uval ba __attribute__ ((unused)))
{
	if (nargs == 5) {
		if (nrets == 1) {
			uval conf_addr = argp[0];
			uval buid;
			uval sz = argp[3];
			uval val = argp[4];
			sval rc;

			buid = argp[1] << ((sizeof (uval) - 4) * 8);
			buid |= argp[2];

			rc = hcall_pci_config_write(NULL, conf_addr, buid,
						    sz, val);
			if (rc == H_Success) {
				retp[0] = RTAS_SUCCEED;
				return RTAS_SUCCEED;
			}
			retp[0] = RTAS_HW_ERROR;
			return RTAS_HW_ERROR;
		}
	}
	retp[0] = RTAS_PARAM_ERROR;
	return RTAS_PARAM_ERROR;
}
			
			
