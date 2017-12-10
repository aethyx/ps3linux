
/*
 * PS3 Jupiter AP
 *
 * Copyright (C) 2011, 2012 glevand <geoffrey.levand@mail.ru>
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <endian.h>
#include <net/ethernet.h>

#include <libusb-1.0/libusb.h>

#include "ps3_eurus.h"
#include "ps3_jupiter.h"
#include "util.h"

#define PS3_JUPITER_AP_USB_VENDOR_ID		0x054c
#define PS3_JUPITER_AP_USB_PRODUCT_ID		0x036f
#define PS3_JUPITER_AP_USB_IFACE		5
#define PS3_JUPITER_AP_USB_EP7			7

#define PS3_JUPITER_AP_RX_XFER_BUFSIZE		0x620

static int ps3_jupiter_ap_initialized = 0;

static libusb_context *ps3_jupiter_ap_usb_ctx;
static libusb_device_handle *ps3_jupiter_ap_usb_dev_handle;

static struct libusb_transfer *ps3_jupiter_ap_rx_xfer;
static unsigned char ps3_jupiter_ap_rx_xfer_buf[PS3_JUPITER_AP_RX_XFER_BUFSIZE];

static pthread_t ps3_jupiter_ap_event_thread;
static int ps3_jupiter_ap_event_thread_done = 0;

static const unsigned char ps3_jupiter_ap_mac_addr[] = {
	0xf0, 0x0d, 0xb0, 0x0b, 0x00, 0x01
};

static const uint64_t ps3_jupiter_ap_channel_info = 0x1fffull << 48;

static pthread_mutex_t ps3_jupiter_ap_dev_init_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ps3_jupiter_ap_dev_init_cond = PTHREAD_COND_INITIALIZER;
static int ps3_jupiter_ap_dev_init_event = 0;

static volatile int ps3_jupiter_ap_done = 0;

/*
 * ps3_jupiter_ap_event_thread_proc
 */
static void *ps3_jupiter_ap_event_thread_proc(void *arg)
{
	int err;

	while (!ps3_jupiter_ap_event_thread_done) {
		err = libusb_handle_events(ps3_jupiter_ap_usb_ctx);
		if (err)
			fprintf(stderr, "libusb_handle_events failed (%d)\n", err);
	}

	return (NULL);
}

/*
 * ps3_jupiter_ap_rx_cb
 */
static void ps3_jupiter_ap_rx_cb(struct libusb_transfer *transfer)
{
	int err;

	switch (transfer->status) {
	case LIBUSB_TRANSFER_COMPLETED:
		fprintf(stdout, "RX transfer completed with length %d\n", transfer->actual_length);
		hexdump(transfer->buffer, transfer->actual_length);
	break;
	case LIBUSB_TRANSFER_CANCELLED:
	case LIBUSB_TRANSFER_NO_DEVICE:
		return;
	default:
		fprintf(stderr, "RX transfer failed (%d)\n", transfer->status);
	}

	libusb_fill_bulk_transfer(ps3_jupiter_ap_rx_xfer, ps3_jupiter_ap_usb_dev_handle,
		LIBUSB_ENDPOINT_IN | PS3_JUPITER_AP_USB_EP7, ps3_jupiter_ap_rx_xfer_buf,
		sizeof(ps3_jupiter_ap_rx_xfer_buf), ps3_jupiter_ap_rx_cb, NULL, 0);

	err = libusb_submit_transfer(ps3_jupiter_ap_rx_xfer);
	if (err)
		fprintf(stderr, "libusb_submit_transfer failed (%d)\n", err);
}

/*
 * ps3_jupiter_ap_init
 */
static int ps3_jupiter_ap_init(int debug_level)
{
	int err;

	err = libusb_init(&ps3_jupiter_ap_usb_ctx);
	if (err) {
		fprintf(stderr, "libusb_init failed (%d)\n", err);
		return (-1);
	}

	libusb_set_debug(ps3_jupiter_ap_usb_ctx, debug_level);

	ps3_jupiter_ap_usb_dev_handle = libusb_open_device_with_vid_pid(ps3_jupiter_ap_usb_ctx,
		PS3_JUPITER_AP_USB_VENDOR_ID, PS3_JUPITER_AP_USB_PRODUCT_ID);
	if (!ps3_jupiter_ap_usb_dev_handle) {
		fprintf(stderr, "libusb_open_device_with_vid_pid failed\n");
		goto fail_destroy_usb_ctx;
	}

	if (libusb_kernel_driver_active(ps3_jupiter_ap_usb_dev_handle, PS3_JUPITER_AP_USB_IFACE)) {
		fprintf(stdout, "kernel driver is attached, trying to detach it\n");

		err = libusb_detach_kernel_driver(ps3_jupiter_ap_usb_dev_handle,
			PS3_JUPITER_AP_USB_IFACE);
		if (err) {
			fprintf(stderr, "libusb_detach_kernel_driver failed (%d)\n", err);
			goto fail_close_usb_device;
		}

		fprintf(stdout, "kernel driver successfully dettached\n");
	}

	err = libusb_claim_interface(ps3_jupiter_ap_usb_dev_handle, PS3_JUPITER_AP_USB_IFACE);
	if (err) {
		fprintf(stderr, "libusb_claim_interface failed (%d)\n", err);
		goto fail_close_usb_device;
	}

	ps3_jupiter_ap_rx_xfer = libusb_alloc_transfer(0);
	if (!ps3_jupiter_ap_rx_xfer) {
		fprintf(stderr, "libusb_alloc_transfer\n");
		goto fail_release_usb_iface;
	}

	libusb_fill_bulk_transfer(ps3_jupiter_ap_rx_xfer, ps3_jupiter_ap_usb_dev_handle,
		LIBUSB_ENDPOINT_IN | PS3_JUPITER_AP_USB_EP7, ps3_jupiter_ap_rx_xfer_buf,
		sizeof(ps3_jupiter_ap_rx_xfer_buf), ps3_jupiter_ap_rx_cb, NULL, 0);

	err = libusb_submit_transfer(ps3_jupiter_ap_rx_xfer);
	if (err) {
		fprintf(stderr, "libusb_submit_transfer (%d)\n", err);
		goto fail_free_usb_rx_xfer;
	}

	ps3_jupiter_ap_event_thread_done = 0;

	err = pthread_create(&ps3_jupiter_ap_event_thread, NULL, ps3_jupiter_ap_event_thread_proc,
		NULL);
	if (err) {
		fprintf(stderr, "pthread_create failed (%d)\n", err);
		goto fail_cancel_usb_rx_xfer;
	}

	ps3_jupiter_ap_initialized = 1;

	return (0);

fail_cancel_usb_rx_xfer:

	libusb_cancel_transfer(ps3_jupiter_ap_rx_xfer);

fail_free_usb_rx_xfer:

	libusb_free_transfer(ps3_jupiter_ap_rx_xfer);

fail_release_usb_iface:

	libusb_release_interface(ps3_jupiter_ap_usb_dev_handle, PS3_JUPITER_AP_USB_IFACE);

fail_close_usb_device:

	libusb_close(ps3_jupiter_ap_usb_dev_handle);

fail_destroy_usb_ctx:

	libusb_exit(ps3_jupiter_ap_usb_ctx);

	return (-1);
}

/*
 * ps3_jupiter_ap_fini
 */
static void ps3_jupiter_ap_fini(void)
{
	if (!ps3_jupiter_ap_initialized)
		return;

	ps3_jupiter_ap_event_thread_done = 1;

	libusb_cancel_transfer(ps3_jupiter_ap_rx_xfer);
	libusb_free_transfer(ps3_jupiter_ap_rx_xfer);

	libusb_close(ps3_jupiter_ap_usb_dev_handle);
	libusb_exit(ps3_jupiter_ap_usb_ctx);

	ps3_jupiter_ap_initialized = 0;
}

/*
 * ps3_jupiter_ap_event
 */
static void ps3_jupiter_ap_event(struct ps3_jupiter_event_listener *self,
	struct ps3_eurus_event *event)
{
	fprintf(stderr, "got event (0x%08x 0x%08x 0x%08x 0x%08x 0x%08x)\n",
		event->hdr.type, event->hdr.id, event->hdr.timestamp, event->hdr.payload_length,
		event->hdr.unknown);

	if (event->hdr.type == PS3_EURUS_EVENT_TYPE_0x400) {
		if ((event->hdr.id == 0x8) || (event->hdr.id == 0x10)) {
			pthread_mutex_lock(&ps3_jupiter_ap_dev_init_lock);
			ps3_jupiter_ap_dev_init_event = 1;
			pthread_cond_signal(&ps3_jupiter_ap_dev_init_cond);
			pthread_mutex_unlock(&ps3_jupiter_ap_dev_init_lock);
		}
	}
				
}

static struct ps3_jupiter_event_listener ps3_jupiter_ap_event_listener = {
	.callback = ps3_jupiter_ap_event,
};

/*
 * ps3_jupiter_ap_dev_init
 */
static int ps3_jupiter_ap_dev_init(void)
{
	struct ps3_eurus_cmd_0x114f *eurus_cmd_0x114f;
	struct ps3_eurus_cmd_0x116f *eurus_cmd_0x116f;
	struct ps3_eurus_cmd_0x115b *eurus_cmd_0x115b;
	const unsigned char bcast_addr[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	unsigned char h;
	struct ps3_eurus_cmd_mcast_addr_filter *eurus_cmd_mcast_addr_filter;
	struct ps3_eurus_cmd_0x110d *eurus_cmd_0x110d;
	struct ps3_eurus_cmd_0x1031 *eurus_cmd_0x1031;
	struct ps3_eurus_cmd_set_mac_addr *eurus_cmd_set_mac_addr;
	struct ps3_eurus_cmd_set_antenna *eurus_cmd_set_antenna;
	struct ps3_eurus_cmd_0x110b *eurus_cmd_0x110b;
	struct ps3_eurus_cmd_0x1109 *eurus_cmd_0x1109;
	struct ps3_eurus_cmd_0x207 *eurus_cmd_0x207;
	struct ps3_eurus_cmd_0x203 *eurus_cmd_0x203;
	struct ps3_eurus_cmd_0x105f *eurus_cmd_0x105f;
	struct ps3_eurus_cmd_get_fw_version *eurus_cmd_get_fw_version;
	unsigned char buf[2048];
	unsigned int status, response_length;
	int err;

	/* state 1 */

	eurus_cmd_0x114f = (struct ps3_eurus_cmd_0x114f *) buf;
	memset(eurus_cmd_0x114f, 0, sizeof(*eurus_cmd_0x114f));

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x114f,
		eurus_cmd_0x114f, sizeof(*eurus_cmd_0x114f), &status, NULL, NULL);
	if (err)
		return (-1);

	/* do not check command status here !!! */

	/* state 2 */

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x1171, NULL, 0, &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	/* state 3 */

	pthread_mutex_lock(&ps3_jupiter_ap_dev_init_lock);
	while (!ps3_jupiter_ap_dev_init_event)
		pthread_cond_wait(&ps3_jupiter_ap_dev_init_cond, &ps3_jupiter_ap_dev_init_lock);
	pthread_mutex_unlock(&ps3_jupiter_ap_dev_init_lock);

	/* state 4 */

	eurus_cmd_0x116f = (struct ps3_eurus_cmd_0x116f *) buf;
	memset(eurus_cmd_0x116f, 0, sizeof(*eurus_cmd_0x116f));
	eurus_cmd_0x116f->unknown = htole32(0x1);

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x116f,
		eurus_cmd_0x116f, sizeof(*eurus_cmd_0x116f), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	/* state 5 */

	eurus_cmd_0x115b = (struct ps3_eurus_cmd_0x115b *) buf;
	memset(eurus_cmd_0x115b, 0, sizeof(*eurus_cmd_0x115b));
	eurus_cmd_0x115b->unknown1 = htole16(0x1);
	eurus_cmd_0x115b->unknown2 = htole16(0x0);
	memcpy(eurus_cmd_0x115b->mac_addr, ps3_jupiter_ap_mac_addr,
		sizeof(ps3_jupiter_ap_mac_addr));

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x115b,
		eurus_cmd_0x115b, sizeof(*eurus_cmd_0x115b), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	/* state 6 */

	h = ps3_eurus_mcast_addr_hash(bcast_addr);

	eurus_cmd_mcast_addr_filter = (struct ps3_eurus_cmd_mcast_addr_filter *) buf;
	memset(eurus_cmd_mcast_addr_filter, 0, sizeof(*eurus_cmd_mcast_addr_filter));
	eurus_cmd_mcast_addr_filter->word[PS3_EURUS_MCAST_ADDR_HASH2POS(h)] |=
	    htole32(PS3_EURUS_MCAST_ADDR_HASH2VAL(h));

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_SET_MCAST_ADDR_FILTER,
		eurus_cmd_mcast_addr_filter, sizeof(*eurus_cmd_mcast_addr_filter),
	    &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	/* state 7 */

	eurus_cmd_0x110d = (struct ps3_eurus_cmd_0x110d *) buf;
	memset(eurus_cmd_0x110d, 0, sizeof(*eurus_cmd_0x110d));
	eurus_cmd_0x110d->unknown1 = htole32(0xffffffff);
	eurus_cmd_0x110d->unknown2 = htole32(0xffffffff);
	eurus_cmd_0x110d->unknown3 = htole32(0xffffffff);
	eurus_cmd_0x110d->unknown4 = htole32(0xffffffff);
	eurus_cmd_0x110d->unknown5 = htole32(0xffffffff);
	eurus_cmd_0x110d->unknown6 = htole32(0xffffffff);
	eurus_cmd_0x110d->unknown7 = htole32(0xffffffff);

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x110d,
		eurus_cmd_0x110d, sizeof(*eurus_cmd_0x110d), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	/* state 8 */

	eurus_cmd_0x1031 = (struct ps3_eurus_cmd_0x1031 *) buf;
	memset(eurus_cmd_0x1031, 0, sizeof(*eurus_cmd_0x1031));
	eurus_cmd_0x1031->unknown1 = 0x0;
	eurus_cmd_0x1031->unknown2 = 0x0;

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x1031,
		eurus_cmd_0x1031, sizeof(*eurus_cmd_0x1031), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	/* state 9 */

	eurus_cmd_set_mac_addr = (struct ps3_eurus_cmd_set_mac_addr *) buf;
	memset(eurus_cmd_set_mac_addr, 0, sizeof(*eurus_cmd_set_mac_addr));
	memcpy(eurus_cmd_set_mac_addr->mac_addr, ps3_jupiter_ap_mac_addr,
		sizeof(ps3_jupiter_ap_mac_addr));

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_SET_MAC_ADDR,
		eurus_cmd_set_mac_addr, sizeof(*eurus_cmd_set_mac_addr), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	/* state 10 */

	eurus_cmd_set_antenna = (struct ps3_eurus_cmd_set_antenna *) buf;
	memset(eurus_cmd_set_antenna, 0, sizeof(*eurus_cmd_set_antenna));

	if ((((ps3_jupiter_ap_channel_info) >> 40) & 0xff) == 0x1) {
		eurus_cmd_set_antenna->unknown1 = 0x1;
		eurus_cmd_set_antenna->unknown2 = 0x0;
	} else if (((ps3_jupiter_ap_channel_info >> 40) & 0xff) == 0x2) {
		eurus_cmd_set_antenna->unknown1 = 0x1;
		eurus_cmd_set_antenna->unknown2 = 0x1;
	} else {
		eurus_cmd_set_antenna->unknown1 = 0x2;
		eurus_cmd_set_antenna->unknown2 = 0x2;
	}

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_SET_ANTENNA,
		eurus_cmd_set_antenna, sizeof(*eurus_cmd_set_antenna), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	/* state 11 */

	eurus_cmd_0x110b = (struct ps3_eurus_cmd_0x110b *) buf;
	memset(eurus_cmd_0x110b, 0, sizeof(*eurus_cmd_0x110b));
	eurus_cmd_0x110b->unknown1 = htole32(0x1);
	eurus_cmd_0x110b->unknown2 = htole32(0x200000);

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x110b,
		eurus_cmd_0x110b, sizeof(*eurus_cmd_0x110b), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	/* state 12 */

	eurus_cmd_0x1109 = (struct ps3_eurus_cmd_0x1109 *) buf;
	ps3_eurus_make_cmd_0x1109(eurus_cmd_0x1109, 0x1, 0x0, 0x2715, 0x9, 0x12);

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x1109,
		eurus_cmd_0x1109, sizeof(*eurus_cmd_0x1109), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	/* state 13 */

	eurus_cmd_0x207 = (struct ps3_eurus_cmd_0x207 *) buf;
	memset(eurus_cmd_0x207, 0, sizeof(*eurus_cmd_0x207));
	eurus_cmd_0x207->unknown = htole32(0x1);

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x207,
		eurus_cmd_0x207, sizeof(*eurus_cmd_0x207), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	/* state 14 */

	eurus_cmd_0x203 = (struct ps3_eurus_cmd_0x203 *) buf;
	memset(eurus_cmd_0x203, 0, sizeof(*eurus_cmd_0x203));
	eurus_cmd_0x203->unknown = htole32(0x1);

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x203,
		eurus_cmd_0x203, sizeof(*eurus_cmd_0x203), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	/* state 15 */

	eurus_cmd_0x105f = (struct ps3_eurus_cmd_0x105f *) buf;
	memset(eurus_cmd_0x105f, 0, sizeof(*eurus_cmd_0x105f));
	eurus_cmd_0x105f->channel_info = htole16(ps3_jupiter_ap_channel_info >> 48);
	memcpy(eurus_cmd_0x105f->mac_addr, ps3_jupiter_ap_mac_addr,
		sizeof(ps3_jupiter_ap_mac_addr));

	if (((ps3_jupiter_ap_channel_info >> 40) & 0xff) == 0x1) {
		eurus_cmd_0x105f->unknown1 = 0x1;
		eurus_cmd_0x105f->unknown2 = 0x0;
	} else if (((ps3_jupiter_ap_channel_info >> 40) & 0xff) == 0x2) {
		eurus_cmd_0x105f->unknown1 = 0x1;
		eurus_cmd_0x105f->unknown2 = 0x1;
	} else {
		eurus_cmd_0x105f->unknown1 = 0x2;
		eurus_cmd_0x105f->unknown2 = 0x2;
	}

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x105f,
		eurus_cmd_0x105f, sizeof(*eurus_cmd_0x105f), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	/* device is ready now */

	/* read firmware version */

	eurus_cmd_get_fw_version = (struct ps3_eurus_cmd_get_fw_version *) buf;
	memset(eurus_cmd_get_fw_version, 0, sizeof(*eurus_cmd_get_fw_version));

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_GET_FW_VERSION,
		eurus_cmd_get_fw_version, sizeof(*eurus_cmd_get_fw_version),
	    &status, &response_length, eurus_cmd_get_fw_version);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	fprintf(stdout, "firmware version: %s\n", (char *) eurus_cmd_get_fw_version->version);

	return (0);
}

/*
 * ps3_jupiter_ap_start
 */
static int ps3_jupiter_ap_start(const char *ssid, unsigned int chan)
{
	struct ps3_eurus_cmd_set_channel *eurus_cmd_set_channel;
	struct ps3_eurus_cmd_0x1d9 *eurus_cmd_0x1d9;
	struct ps3_eurus_cmd_ap_opmode *eurus_cmd_ap_opmode;
	struct ps3_eurus_cmd_ap_ssid *eurus_cmd_ap_ssid;
	struct ps3_eurus_cmd_0x1ed *eurus_cmd_0x1ed;
	struct ps3_eurus_cmd_0x1031 *eurus_cmd_0x1031;
	struct ps3_eurus_cmd_0x104d *eurus_cmd_0x104d;
	struct ps3_eurus_cmd_ap_wep_config *eurus_cmd_ap_wep_config;
	struct ps3_eurus_cmd_0x61 *eurus_cmd_0x61;
	struct ps3_eurus_cmd_0xc5 *eurus_cmd_0xc5;
	struct ps3_eurus_cmd_0x1 *eurus_cmd_0x1;
	struct ps3_eurus_cmd_start_ap *eurus_cmd_start_ap;
	unsigned char buf[2048];
	unsigned int status;
	int err;

	eurus_cmd_set_channel = (struct ps3_eurus_cmd_set_channel *) buf;
	memset(eurus_cmd_set_channel, 0, sizeof(*eurus_cmd_set_channel));
	eurus_cmd_set_channel->channel = chan;

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_SET_CHANNEL,
		eurus_cmd_set_channel, sizeof(*eurus_cmd_set_channel), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	eurus_cmd_0x1d9 = (struct ps3_eurus_cmd_0x1d9 *) buf;
	memset(eurus_cmd_0x1d9, 0, sizeof(*eurus_cmd_0x1d9));
	eurus_cmd_0x1d9->unknown1 = 0x1;

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x1d9,
		eurus_cmd_0x1d9, sizeof(*eurus_cmd_0x1d9), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	eurus_cmd_ap_opmode = (struct ps3_eurus_cmd_ap_opmode *) buf;
	memset(eurus_cmd_ap_opmode, 0, sizeof(*eurus_cmd_ap_opmode));
	eurus_cmd_ap_opmode->opmode = htole32(PS3_EURUS_AP_OPMODE_11BG);

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_SET_AP_OPMODE,
		eurus_cmd_ap_opmode, sizeof(*eurus_cmd_ap_opmode), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	eurus_cmd_ap_ssid = (struct ps3_eurus_cmd_ap_ssid *) buf;
	memset(eurus_cmd_ap_ssid, 0, sizeof(*eurus_cmd_ap_ssid));
	strncpy((char *) eurus_cmd_ap_ssid->ssid, ssid, sizeof(eurus_cmd_ap_ssid->ssid));

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_SET_AP_SSID,
		eurus_cmd_ap_ssid, sizeof(*eurus_cmd_ap_ssid), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	eurus_cmd_0x1ed = (struct ps3_eurus_cmd_0x1ed *) buf;
	memset(eurus_cmd_0x1ed, 0, sizeof(*eurus_cmd_0x1ed));
	eurus_cmd_0x1ed->unknown1 = htole32(0x1);
	eurus_cmd_0x1ed->unknown2 = 0x1;
	eurus_cmd_0x1ed->unknown3 = 0x2;
	eurus_cmd_0x1ed->unknown4 = 0xff;
	eurus_cmd_0x1ed->unknown5 = 0x4;
	eurus_cmd_0x1ed->unknown6 = 0x4;
	eurus_cmd_0x1ed->unknown7 = 0xa;
	eurus_cmd_0x1ed->unknown8 = 0x4;

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x1ed,
		eurus_cmd_0x1ed, sizeof(*eurus_cmd_0x1ed), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	eurus_cmd_0x1031 = (struct ps3_eurus_cmd_0x1031 *) buf;
	memset(eurus_cmd_0x1031, 0, sizeof(*eurus_cmd_0x1031));
	eurus_cmd_0x1031->unknown1 = 0x1;

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x1031,
		eurus_cmd_0x1031, sizeof(*eurus_cmd_0x1031), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	eurus_cmd_0x104d = (struct ps3_eurus_cmd_0x104d *) buf;
	memset(eurus_cmd_0x104d, 0, sizeof(*eurus_cmd_0x104d));
	eurus_cmd_0x104d->unknown = 0x0;

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x104d,
		eurus_cmd_0x104d, sizeof(*eurus_cmd_0x104d), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	eurus_cmd_ap_wep_config = (struct ps3_eurus_cmd_ap_wep_config *) buf;
	memset(eurus_cmd_ap_wep_config, 0, sizeof(*eurus_cmd_ap_wep_config));
	eurus_cmd_ap_wep_config->security_mode = PS3_EURUS_WEP_SECURITY_NONE;

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_SET_AP_WEP_CONFIG,
		eurus_cmd_ap_wep_config, sizeof(*eurus_cmd_ap_wep_config), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	eurus_cmd_0x61 = (struct ps3_eurus_cmd_0x61 *) buf;
	memset(eurus_cmd_0x61, 0, sizeof(*eurus_cmd_0x61));
	eurus_cmd_0x61->unknown = 0x0;

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x61,
		eurus_cmd_0x61, sizeof(*eurus_cmd_0x61), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	eurus_cmd_0xc5 = (struct ps3_eurus_cmd_0xc5 *) buf;
	memset(eurus_cmd_0xc5, 0, sizeof(*eurus_cmd_0xc5));

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0xc5,
		eurus_cmd_0xc5, sizeof(*eurus_cmd_0xc5), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	eurus_cmd_0x1 = (struct ps3_eurus_cmd_0x1 *) buf;
	memset(eurus_cmd_0x1, 0, sizeof(*eurus_cmd_0x1));
	eurus_cmd_0x1->unknown = 0x1;

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x1,
		eurus_cmd_0x1, sizeof(*eurus_cmd_0x1), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	eurus_cmd_start_ap = (struct ps3_eurus_cmd_start_ap *) buf;
	memset(eurus_cmd_start_ap, 0, sizeof(*eurus_cmd_start_ap));
	eurus_cmd_start_ap->unknown = 0x2;

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_START_AP,
		eurus_cmd_start_ap, sizeof(*eurus_cmd_start_ap), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	return (0);
}

/*
 * ps3_jupiter_ap_stop
 */
static int ps3_jupiter_ap_stop(void)
{
	struct ps3_eurus_cmd_start_ap *eurus_cmd_start_ap;
	unsigned char buf[2048];
	unsigned int status;
	int err;

	eurus_cmd_start_ap = (struct ps3_eurus_cmd_start_ap *) buf;
	memset(eurus_cmd_start_ap, 0, sizeof(*eurus_cmd_start_ap));
	eurus_cmd_start_ap->unknown = 0x1;

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_START_AP,
		eurus_cmd_start_ap, sizeof(*eurus_cmd_start_ap), &status, NULL, NULL);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	return (0);
}

/*
 * ps3_jupiter_ap_print_mac_addr_list
 */
static int ps3_jupiter_ap_print_mac_addr_list(void)
{
	struct ps3_eurus_cmd_get_mac_addr_list *eurus_cmd_get_mac_addr_list;
	unsigned char buf[2048];
	unsigned int status, response_length;
	int i, err;

	eurus_cmd_get_mac_addr_list = (struct ps3_eurus_cmd_get_mac_addr_list *) buf;
	memset(eurus_cmd_get_mac_addr_list, 0, sizeof(*eurus_cmd_get_mac_addr_list));

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_GET_MAC_ADDR_LIST,
		eurus_cmd_get_mac_addr_list, PS3_EURUS_MAC_ADDR_LIST_MAXSIZE, &status,
		&response_length, eurus_cmd_get_mac_addr_list);
	if (err)
		return (-1);

	if (status != PS3_EURUS_CMD_OK)
		return (-1);

	fprintf(stdout, "number of MAC addresses %d\n", le16toh(eurus_cmd_get_mac_addr_list->count));

	for (i = 0; i < le16toh(eurus_cmd_get_mac_addr_list->count); i++)
		fprintf(stdout, "%02x:%02x:%02x:%02x:%02x:%02x\n",
			eurus_cmd_get_mac_addr_list->mac_addr[i * 6 + 0],
			eurus_cmd_get_mac_addr_list->mac_addr[i * 6 + 1],
			eurus_cmd_get_mac_addr_list->mac_addr[i * 6 + 2],
			eurus_cmd_get_mac_addr_list->mac_addr[i * 6 + 3],
			eurus_cmd_get_mac_addr_list->mac_addr[i * 6 + 4],
			eurus_cmd_get_mac_addr_list->mac_addr[i * 6 + 5]);

	return (0);
}

/*
 * signal_handler
 */
static void signal_handler(int signo)
{
	fprintf(stderr, "got signal %d\n", signo);

	switch (signo) {
	case SIGINT:
		ps3_jupiter_ap_done = 1;
	break;
	}
}

/*
 * main
 */
int main(int argc, char **argv)
{
	int err;

	err = ps3_jupiter_init(5);
	if (err) {
		fprintf(stderr, "ps3_jupiter_init failed (%d)\n", err);
		return (1);
	}

	err = ps3_jupiter_register_event_listener(&ps3_jupiter_ap_event_listener);
	if (err) {
		fprintf(stderr, "ps3_jupiter_register_event_listener failed (%d)\n", err);
		err = 1;
		goto fini;
	}

	err = ps3_jupiter_ap_init(5);
	if (err) {
		fprintf(stderr, "ps3_jupiter_ap_init failed (%d)\n", err);
		err = 1;
		goto fini;
	}

	signal(SIGINT, signal_handler);

	err = ps3_jupiter_ap_dev_init();
	if (err) {
		fprintf(stderr, "ps3_jupiter_ap_dev_init failed (%d)\n", err);
		err = 1;
		goto ap_fini;
	}

	err = ps3_jupiter_ap_start("wlan_rockz", 6);
	if (err) {
		fprintf(stderr, "ps3_jupiter_ap_start failed (%d)\n", err);
		err = 1;
		goto ap_stop;
	}

	err = ps3_jupiter_ap_print_mac_addr_list();
	if (err) {
		fprintf(stderr, "ps3_jupiter_ap_print_mac_addr_list failed (%d)\n", err);
		err = 1;
		goto ap_stop;
	}

	while (!ps3_jupiter_ap_done)
		pause();

ap_stop:

	ps3_jupiter_ap_stop();

ap_fini:

	ps3_jupiter_ap_fini();

fini:

	ps3_jupiter_fini();

	return (err);
}
