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
#include <ofh.h>
#include <lib.h>
#include <rtas.h>
#include <rh.h>
#include <util.h>

struct rh_args_s {
	uval	rha_token;
	uval	rha_nargs;
	uval	rha_nrets;
	uval	rha_args[0];
} __attribute__ ((aligned (8)));;

static rh_func_t *rf[] = {
	[nvram_fetch]			= rh_nvram_fetch,
	[nvram_store]			= rh_nvram_store,
	[get_time_of_day]		= rh_get_time_of_day,
	[set_time_of_day]		= rh_set_time_of_day,
	[set_time_for_power_on]		= rh_set_time_for_power_on,
	[event_scan]			= rh_event_scan,
	[check_exception]		= rh_check_exception,
	[read_pci_config]		= rh_read_pci_config,
	[write_pci_config]		= rh_write_pci_config,
	[display_character]		= rh_display_character,
	[set_indicator]			= rh_set_indicator,
	[get_sensor_state]		= rh_get_sensor_state,
	[set_power_level]		= rh_set_power_level,
	[get_power_level]		= rh_get_power_level,
	[relinquish_power_management]	= rh_relinquish_power_management,
	[power_off]			= rh_power_off,
	[system_reboot]			= rh_system_reboot,
	[stop_self]			= rh_stop_self,
	[start_cpu]			= rh_start_cpu,
	[ibm_os_term]			= rh_ibm_os_term,
	[ibm_get_xive]			= rh_ibm_get_xive,
	[ibm_set_xive]			= rh_ibm_set_xive,
	[ibm_int_on]			= rh_ibm_int_on,
	[ibm_int_off]			= rh_ibm_int_off,
	[ibm_open_errinjct]		= rh_ibm_open_errinjct,
	[ibm_errinjct]			= rh_ibm_errinjct,
	[ibm_close_errinjct]		= rh_ibm_close_errinjct,
	[ibm_configure_connector]	= rh_ibm_configure_connector,
	[ibm_set_eeh_option]		= rh_ibm_set_eeh_option,
	[ibm_set_slot_reset]		= rh_ibm_set_slot_reset,
	[ibm_read_slot_reset_state]	= rh_ibm_read_slot_reset_state,
	[ibm_read_pci_config]		= rh_ibm_read_pci_config,
	[ibm_write_pci_config]		= rh_ibm_write_pci_config,
	[ibm_nmi_interlock]		= rh_ibm_nmi_interlock,
	[ibm_get_system_parameter]	= rh_ibm_get_system_parameter,
	[ibm_set_system_parameter]	= rh_ibm_set_system_parameter,
	[ibm_display_message]		= rh_ibm_display_message,
	[ibm_platform_dump]		= rh_ibm_platform_dump,
	[ibm_manage_flash_image]	= rh_ibm_manage_flash_image,
	[ibm_validate_flash_image]	= rh_ibm_validate_flash_image,
};

sval
rh_handler(uval rh_args, uval rh_data)
{
	struct rh_args_s *rha = (struct rh_args_s *)rh_args;
	uval token = rha->rha_token;
	rh_func_t *fp;

	fp = *(DRELA(&rf[token], rh_data));
	/* check if the funtion pointer is empty */
	if (fp == NULL) {
		/* not sure what I'm supposed for funtion return so I
		 * do both */
		rha->rha_args[rha->rha_nargs] = RTAS_PARAM_ERROR;
		return RTAS_PARAM_ERROR;
	}
	return leap(rha->rha_nargs, rha->rha_nrets,
		       rha->rha_args, &rha->rha_args[rha->rha_nargs],
		       rh_data, (uval)fp);
}



extern sval rh_start(uval rh_args, uval rh_data);
extern void _start(void);
void
_start(void)
{
	struct rh_args_s *args;
	uval argsz;
	uval i;

	argsz = sizeof (*args);
	argsz += sizeof (args->rha_args[0]) * 16;
	args = alloca(argsz);

	args->rha_token = display_character;
	args->rha_nargs = 1;
	args->rha_nrets = 1;

	for (i = 'a'; i < ('z' + 1); i++) {
		args->rha_args[0] = i;
		rh_start((uval)args, 0);
	}
	args->rha_args[0] = '\r';
	rh_start((uval)args, 0);
	args->rha_args[0] = '\n';
	rh_start((uval)args, 0);
}
