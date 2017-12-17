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
 *
 */

#include <config.h>
#include <rh.h>
#include <hcall.h>

uval
rh_get_tod(struct rh_tm_s *tm, uval ba)
{
	tm->rtm_year = 103;
	tm->rtm_month = 12;
	tm->rtm_day = 25;
	tm->rtm_hour = 9;
	tm->rtm_min = 0;
	tm->rtm_sec = 0;
	tm->rtm_nsec = 0;

	rh_adjust_tod(tm, ba);

	return 1;
}



sval
rh_get_time_of_day(uval nargs, uval nrets,
		   uval argp[] __attribute__ ((unused)),
		   uval retp[], uval ba)
{
	if (nargs == 0) {
		if (nrets == 8) {
			struct rh_tm_s *tm = (struct rh_tm_s *)&retp[1];
			if (rh_get_tod(tm, ba)) {
				retp[0] = RTAS_SUCCEED;
				return RTAS_SUCCEED;
			}
		}
	}
	retp[0] = RTAS_PARAM_ERROR;
	return RTAS_PARAM_ERROR;
}
