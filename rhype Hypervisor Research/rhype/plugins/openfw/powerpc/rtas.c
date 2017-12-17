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

#include <ofh.h>
#include <lib.h>
#include <rtas.h>
#include <util.h>

extern char _rtas_image32_start[];
extern char _rtas_image32_end[];
extern char _rtas_image32_size[];
extern char _rtas_image64_start[];
extern char _rtas_image64_end[];
extern char _rtas_image64_size[];

static sval
rtas_instantiate_rtas(uval nargs, uval nrets, sval argp[], sval retp[], uval b)
{
	if (nargs == 1) {
		if (nrets == 1) {
			void *rtas_base_address = (void *)argp[0];
			uval sz = (_rtas_image32_end - _rtas_image32_start);

			memcpy(rtas_base_address,
			       DRELA(&_rtas_image32_start[0], b), sz);
			retp[0] = (uval)rtas_base_address;
			
			return OF_SUCCESS;
		}
	}
	return OF_FAILURE;
}


static struct ofh_methods_s _rtas_methods[] = {
	{ "instantiate-rtas", rtas_instantiate_rtas },
	{ NULL, NULL},
};

static struct ofh_ihandle_s _ih_rtas = {
	.ofi_methods = _rtas_methods,
};

static uval
rtas_open(uval b)
{
	uval ih = DRELA((uval)&_ih_rtas, b);

	return ih;
}


#define PROP(s,v) \
	do { \
		uval a = v; \
		static const char tok[] = s; \
		ofd_prop_add(m, n, DRELA(&tok[0], b), &a, sizeof (a)); \
	} while (0)

void 
ofh_rtas_init(uval b)
{
	static const uval termno[] = {0x0, 0x1};
	static const uval version = 1;
	static const char path[] = "/rtas";
	static const char hypertas[] = "dummy";
	ofdn_t p;
	ofdn_t n;
	void *m = ofd_mem(b);
	uval sz;


	p = ofd_node_find(m, DRELA((const char *)"/", b));

	n = ofd_node_add(m, p, DRELA(&path[0], b), sizeof(path));
	ofd_prop_add(m, n, DRELA((const char *)"name", b),
		     DRELA(&path[1], b), sizeof (path) - 1);
	ofd_prop_add(m, n,
		     DRELA((const char *)"ibm,hypertas-functions", b),
		     DRELA(&hypertas[0], b), sizeof (hypertas));

	ofd_prop_add(m, n, DRELA((const char *)"ibm,termno", b),
		     DRELA(&termno[0], b), sizeof (termno));
	ofd_prop_add(m, n, DRELA((const char *)"rtas-version", b),
		     DRELA(&version, b), sizeof (version));


	sz = MAX((uval)_rtas_image64_size, (uval)_rtas_image64_size);

	ofd_prop_add(m, n, DRELA((const char *)"rtas-size", b),
		     &sz, sizeof(sz));

	/* RTAS Tokens */
	PROP("nvram-fetch", nvram_fetch);
	PROP("nvram-store", nvram_store);
	PROP("get-time-of-day", get_time_of_day);
	PROP("set-time-of-day", set_time_of_day);
	PROP("set-time-for-power-on", set_time_for_power_on);
	PROP("event-scan", event_scan);
	PROP("check-exception", check_exception);
	PROP("read-pci-config", read_pci_config);
	PROP("write-pci-config", write_pci_config);
	PROP("display-character", display_character);
	PROP("set-indicator", set_indicator);
	PROP("get-sensor-state", get_sensor_state);
	PROP("set-power-level", set_power_level);
	PROP("get-power-level", get_power_level);
	PROP("relinquish-power-management", relinquish_power_management);
	PROP("power-off", power_off);
	PROP("system-reboot", system_reboot);
	PROP("stop-self", stop_self);
	PROP("start-cpu", start_cpu);
	PROP("ibm,os-term", ibm_os_term);
	PROP("ibm,get-xive", ibm_get_xive);
	PROP("ibm,set-xive", ibm_set_xive);
	PROP("ibm,int-on", ibm_int_on);
	PROP("ibm,int-off", ibm_int_off);
	PROP("ibm,open-errinjct", ibm_open_errinjct);
	PROP("ibm,errinjct", ibm_errinjct);
	PROP("ibm,close-errinjct", ibm_close_errinjct);
	PROP("ibm,configure-connector", ibm_configure_connector);
	PROP("ibm,set-eeh-option", ibm_set_eeh_option);
	PROP("ibm,set-slot-reset", ibm_set_slot_reset);
	PROP("ibm,read-slot-reset-state", ibm_read_slot_reset_state);
	PROP("ibm,read-pci-config", ibm_read_pci_config);
	PROP("ibm,write-pci-config", ibm_write_pci_config);
	PROP("ibm,nmi-interlock", ibm_nmi_interlock);
	PROP("ibm,get-system-parameter", ibm_get_system_parameter);
	PROP("ibm,set-system-parameter", ibm_set_system_parameter);
	PROP("ibm,display-message", ibm_display_message);
	PROP("ibm,platform-dump", ibm_platform_dump);
	PROP("ibm,manage-flash-image", ibm_manage_flash_image);
	PROP("ibm,validate-flash-image", ibm_validate_flash_image);

	/* create an IO node */
	p = ofd_io_create(m, n, (uval64)rtas_open);
}
