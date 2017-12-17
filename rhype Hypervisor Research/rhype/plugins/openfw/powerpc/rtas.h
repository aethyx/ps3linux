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

#ifndef _RTAS_H
#define _RTAS_H

/*
 * These actual values were taken from a a specific machine.  They
 * really should not matter but hey.. why not?
 */

enum rtas_tokens_e {
	nvram_fetch			= 0x0001,
	nvram_store			= 0x0002,
	get_time_of_day			= 0x0003,
	set_time_of_day			= 0x0004,
	set_time_for_power_on		= 0x0005,
	event_scan			= 0x0006,
	check_exception			= 0x0007,
	read_pci_config			= 0x0008,
	write_pci_config		= 0x0009,
	display_character		= 0x000a,
	set_indicator			= 0x000b,
	get_sensor_state		= 0x000c, 
	set_power_level			= 0x000d,  
	get_power_level			= 0x000e, 
	relinquish_power_management	= 0x0010,
	power_off			= 0x0011,
	system_reboot			= 0x0014,
	stop_self			= 0x0018, 
	start_cpu			= 0x0019,
	ibm_os_term			= 0x001a,
	ibm_get_xive			= 0x0022,
	ibm_set_xive			= 0x0023,
	ibm_int_on			= 0x0024,
	ibm_int_off			= 0x0025,
	ibm_open_errinjct		= 0x0026,
	ibm_errinjct			= 0x0027,
	ibm_close_errinjct		= 0x0028,     
	ibm_configure_connector		= 0x002a,       
	ibm_set_eeh_option		= 0x002b,
	ibm_set_slot_reset		= 0x002c,
	ibm_read_slot_reset_state	= 0x002d,        
	ibm_read_pci_config		= 0x002e,    
	ibm_write_pci_config		= 0x002f,
	ibm_nmi_interlock		= 0x0031,
	ibm_get_system_parameter	= 0x0032,
	ibm_set_system_parameter	= 0x0034,
	ibm_display_message		= 0x003a,
	ibm_platform_dump		= 0x003b,
	ibm_manage_flash_image		= 0x003c,
	ibm_validate_flash_image	= 0x003d,
};

extern struct ofh_methods_s *rtas_methods(uval b);

#endif /* _RTAS_H */


