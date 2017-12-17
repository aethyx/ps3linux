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
#include <util.h>

static struct rh_tm_s tm_scratch;
static struct rh_tm_s tm_adjust;

void
rh_adjust_tod(struct rh_tm_s *tm, uval ba)
{
	struct rh_tm_s *tma = DRELA(&tm_adjust, ba);

	tm->rtm_sec += tma->rtm_sec;
	if (tm->rtm_sec > 59) {
		tm->rtm_sec -= 60;
		++tm->rtm_min;
	}

	tm->rtm_min += tma->rtm_min;
	if (tm->rtm_sec > 59) {
		tm->rtm_min -= 60;
		++tm->rtm_hour;
	}

	tm->rtm_hour += tma->rtm_hour;
	if (tm->rtm_hour > 23) {
		tm->rtm_hour -= 24;
		++tm->rtm_day;
	}

	/* FIXME: ok things fall apart here but we will come back to
	 * this */
	tm->rtm_day += tma->rtm_day;
	if (tm->rtm_day > 28) {
		tm->rtm_day -= 28;
		++tm->rtm_month;
	}

	tm->rtm_month += tma->rtm_month;
	if (tm->rtm_month > 12) {
		tm->rtm_month -= 12;
		++tm->rtm_year;
	}

	tm->rtm_year += tma->rtm_year;
}

sval
rh_set_time_of_day(uval nargs, uval nrets, uval argp[], uval retp[], uval ba)
{
	if (nargs == 7) {
		if (nrets == 1) {
			struct rh_tm_s *tms = DRELA(&tm_scratch, ba);
			struct rh_tm_s *tm = (struct rh_tm_s *)&argp[0];
			if (rh_get_tod(tms, ba)) {
				struct rh_tm_s *tma = DRELA(&tm_adjust, ba);

				tma->rtm_year = tms->rtm_year - tm->rtm_year;
				tma->rtm_month = tms->rtm_month -
					tm->rtm_month;
				tma->rtm_day = tms->rtm_day - tm->rtm_day;
				tma->rtm_hour = tms->rtm_hour - tm->rtm_hour;
				tma->rtm_min = tms->rtm_min - tm->rtm_min;
				tm->rtm_sec = tms->rtm_sec - tm->rtm_sec;

				retp[0] = RTAS_SUCCEED;
				return RTAS_SUCCEED;
			}
		}
	}
	retp[0] = RTAS_PARAM_ERROR;
	return RTAS_PARAM_ERROR;
}
