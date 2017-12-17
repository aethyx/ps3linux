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

#ifndef _RH_H
#define _RH_H

#include <config.h>
#include <types.h>

#define RTAS_SUCCEED		 0
#define RTAS_HW_ERROR		-1
#define RTAS_HW_BUSY		-2
#define RTAS_PARAM_ERROR	-3
#define RTAS_EXTENED_DELAY	9900  /* range is 9900-9905 */
#define RTAS_MLI		-9000 /* Multi-level isolation error */

/*
 * As we add the required services, their weak definition should be
 * removed.  Optional services can keep the weak definition so we can
 * optionally compile them in.
 */
typedef sval (rh_func_t)(uval, uval, uval [], uval [], uval);
extern rh_func_t rh_nvram_fetch __attribute__ ((weak));		      
extern rh_func_t rh_nvram_store __attribute__ ((weak));		      
extern rh_func_t rh_get_time_of_day __attribute__ ((weak));		      
extern rh_func_t rh_set_time_of_day __attribute__ ((weak));		      
extern rh_func_t rh_set_time_for_power_on __attribute__ ((weak));	      
extern rh_func_t rh_event_scan __attribute__ ((weak));		      
extern rh_func_t rh_check_exception __attribute__ ((weak));		      
extern rh_func_t rh_display_character __attribute__ ((weak));	      
extern rh_func_t rh_set_indicator __attribute__ ((weak));		      
extern rh_func_t rh_get_sensor_state __attribute__ ((weak));	      
extern rh_func_t rh_set_power_level __attribute__ ((weak));		      
extern rh_func_t rh_get_power_level __attribute__ ((weak));		      
extern rh_func_t rh_read_pci_config __attribute__ ((weak));	      
extern rh_func_t rh_write_pci_config __attribute__ ((weak));	      
extern rh_func_t rh_relinquish_power_management __attribute__ ((weak));   
extern rh_func_t rh_power_off __attribute__ ((weak));		      
extern rh_func_t rh_system_reboot __attribute__ ((weak));		      
extern rh_func_t rh_stop_self __attribute__ ((weak));		      
extern rh_func_t rh_start_cpu __attribute__ ((weak));		      
extern rh_func_t rh_ibm_os_term __attribute__ ((weak));		      
extern rh_func_t rh_ibm_get_xive __attribute__ ((weak));		      
extern rh_func_t rh_ibm_set_xive __attribute__ ((weak));		      
extern rh_func_t rh_ibm_int_on __attribute__ ((weak));		      
extern rh_func_t rh_ibm_int_off __attribute__ ((weak));		      
extern rh_func_t rh_ibm_open_errinjct __attribute__ ((weak));	      
extern rh_func_t rh_ibm_errinjct __attribute__ ((weak));		      
extern rh_func_t rh_ibm_close_errinjct __attribute__ ((weak));	      
extern rh_func_t rh_ibm_configure_connector __attribute__ ((weak));	      
extern rh_func_t rh_ibm_set_eeh_option __attribute__ ((weak));	      
extern rh_func_t rh_ibm_set_slot_reset __attribute__ ((weak));	      
extern rh_func_t rh_ibm_read_slot_reset_state __attribute__ ((weak));     
extern rh_func_t rh_ibm_read_pci_config __attribute__ ((weak));	      
extern rh_func_t rh_ibm_write_pci_config __attribute__ ((weak));	      
extern rh_func_t rh_ibm_nmi_interlock __attribute__ ((weak));	      
extern rh_func_t rh_ibm_get_system_parameter __attribute__ ((weak));      
extern rh_func_t rh_ibm_set_system_parameter __attribute__ ((weak));      
extern rh_func_t rh_ibm_display_message __attribute__ ((weak));	      
extern rh_func_t rh_ibm_platform_dump __attribute__ ((weak));	      
extern rh_func_t rh_ibm_manage_flash_image __attribute__ ((weak));	      
extern rh_func_t rh_ibm_validate_flash_image __attribute__ ((weak));      

extern uval *rh_base;
extern sval rh_handler(uval rh_arg, uval rh_data);

struct rh_tm_s {
	uval rtm_year;
	uval rtm_month;
	uval rtm_day;
	uval rtm_hour;
	uval rtm_min;
	uval rtm_sec;
	uval rtm_nsec;
};

extern uval rh_get_tod(struct rh_tm_s *tm, uval ba);
extern void rh_adjust_tod(struct rh_tm_s *tm, uval ba);



#endif /* _RH_H */
