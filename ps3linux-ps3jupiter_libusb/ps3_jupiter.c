
/*
 * PS3 Jupiter
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
#include <endian.h>
#include <net/ethernet.h>

#include <libusb-1.0/libusb.h>

#include "ps3_eurus.h"
#include "ps3_jupiter.h"

#define PS3_JUPITER_USB_VENDOR_ID	0x054c
#define PS3_JUPITER_USB_PRODUCT_ID	0x036f
#define PS3_JUPITER_USB_IFACE		3
#define PS3_JUPITER_USB_EP5		5

#define PS3_JUPITER_IRQ_XFER_BUFSIZE	0x800
#define PS3_JUPITER_CMD_XFER_BUFSIZE	0x800

enum ps3_jupiter_pkt_type {
	PS3_JUPITER_PKT_CMD	= 6,
	PS3_JUPITER_PKT_EVENT	= 8,
};

struct ps3_jupiter_pkt_hdr {
	uint8_t unknown1;
	uint8_t unknown2;
	uint8_t type;
} __attribute ((packed));

struct ps3_jupiter_cmd_hdr {
	uint8_t unknown1;
	uint16_t unknown2;
	uint8_t res1[2];
	uint16_t tag;
	uint8_t res2[14];
} __attribute ((packed));

struct ps3_jupiter_event_hdr {
	uint8_t count;
} __attribute ((packed));

static unsigned char ps3_jupiter_devkey[] = {
	0x76, 0x4e, 0x4b, 0x07, 0x24, 0x42, 0x53, 0xfb,
	0x5a, 0xc7, 0xcc, 0x1d, 0xae, 0x00, 0xc6, 0xd8,
	0x14, 0x40, 0x61, 0x8b, 0x13, 0x17, 0x4d, 0x7c,
	0x3b, 0xb6, 0x90, 0xb8, 0x6e, 0x8b, 0xbb, 0x1d,
};

static int ps3_jupiter_initialized = 0;

static libusb_context *ps3_jupiter_usb_ctx;
static libusb_device_handle *ps3_jupiter_usb_dev_handle;

static struct libusb_transfer *ps3_jupiter_irq_xfer;
static unsigned char ps3_jupiter_irq_xfer_buf[PS3_JUPITER_IRQ_XFER_BUFSIZE];

static pthread_mutex_t ps3_jupiter_event_listener_lock = PTHREAD_MUTEX_INITIALIZER;
static struct ps3_jupiter_event_listener ps3_jupiter_event_listener_head;
static pthread_t ps3_jupiter_event_thread;
static int ps3_jupiter_event_thread_done = 0;

static pthread_mutex_t ps3_jupiter_cmd_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ps3_jupiter_cmd_cond = PTHREAD_COND_INITIALIZER;
static struct libusb_transfer *ps3_jupiter_cmd_xfer;
static unsigned char ps3_jupiter_cmd_xfer_buf[PS3_JUPITER_CMD_XFER_BUFSIZE];
static int ps3_jupiter_cmd_busy = 0;
static int ps3_jupiter_cmd_xfer_done = 1;
static int ps3_jupiter_cmd_done = 1;
static int ps3_jupiter_cmd_status;
static uint16_t ps3_jupiter_cmd_tag = 0;
static uint16_t ps3_jupiter_eurus_cmd;
static uint16_t ps3_jupiter_eurus_tag = 0;

/*
 * ps3_jupiter_event_irq
 */
static void ps3_jupiter_event_irq(void *buf, unsigned int length)
{
	struct ps3_jupiter_pkt_hdr *pkt_hdr;
	struct ps3_jupiter_event_hdr *event_hdr;
	struct ps3_eurus_event event;
	struct ps3_jupiter_event_listener *listener;
	int i;

	if (length < sizeof(*pkt_hdr) + sizeof(*event_hdr)) {
		fprintf(stderr, "got event IRQ packet with invalid length (%d)\n",
			length);
		return;
	}

	pkt_hdr = (struct ps3_jupiter_pkt_hdr *) buf;
	event_hdr = (struct ps3_jupiter_event_hdr *) (pkt_hdr + 1);

	if (length < sizeof(*pkt_hdr) + sizeof(*event_hdr) +
		event_hdr->count * sizeof(struct ps3_eurus_event)) {
		fprintf(stderr, "got event IRQ packet with invalid length (%d)\n",
			length);
		return;
	}

	for (i = 0; i < event_hdr->count; i++) {
		memcpy(&event, (unsigned char *) event_hdr + sizeof(*event_hdr) +
			i * sizeof(struct ps3_eurus_event), sizeof(struct ps3_eurus_event));

		event.hdr.type = le32toh(event.hdr.type);
		event.hdr.id = le32toh(event.hdr.id);
		event.hdr.timestamp = le32toh(event.hdr.timestamp);
		event.hdr.payload_length = le32toh(event.hdr.payload_length);
		event.hdr.unknown = le32toh(event.hdr.unknown);

		pthread_mutex_lock(&ps3_jupiter_event_listener_lock);

		for (listener = ps3_jupiter_event_listener_head.next;
			listener != &ps3_jupiter_event_listener_head;
			listener = listener->next) {
			listener->callback(listener, &event);
		}

		pthread_mutex_unlock(&ps3_jupiter_event_listener_lock);
	}
}

/*
 * ps3_jupiter_cmd_irq
 */
static void ps3_jupiter_cmd_irq(void *buf, unsigned int length)
{
	struct ps3_jupiter_pkt_hdr *pkt_hdr;
	struct ps3_jupiter_cmd_hdr *cmd_hdr;
	struct ps3_eurus_cmd_hdr *eurus_cmd_hdr;
	uint16_t cmd_tag, eurus_cmd, eurus_tag, payload_length;

	if (length < sizeof(*pkt_hdr) + sizeof(*cmd_hdr) + sizeof(*eurus_cmd_hdr)) {
		fprintf(stderr, "got command IRQ packet with invalid length (%d)\n",
			length);
		return;
	}

	pkt_hdr = (struct ps3_jupiter_pkt_hdr *) buf;
	cmd_hdr = (struct ps3_jupiter_cmd_hdr *) (pkt_hdr + 1);
	eurus_cmd_hdr = (struct ps3_eurus_cmd_hdr *) (cmd_hdr + 1);
	payload_length = le16toh(eurus_cmd_hdr->payload_length);

	if (length < sizeof(*pkt_hdr) + sizeof(*cmd_hdr) + sizeof(*eurus_cmd_hdr) +
		payload_length) {
		fprintf(stderr, "got command IRQ packet with invalid length (%d)\n",
			length);
		return;
	}

	cmd_tag = le16toh(cmd_hdr->tag);

	if (ps3_jupiter_cmd_tag != cmd_tag)
		fprintf(stderr, "got command IRQ packet with invalid command tag, "
			"got (0x%04x), expected (0x%04x)\n", cmd_tag, ps3_jupiter_cmd_tag);

	eurus_cmd = le16toh(eurus_cmd_hdr->id);

	if ((ps3_jupiter_eurus_cmd + 1) != eurus_cmd)
		fprintf(stderr, "got command IRQ packet with invalid EURUS command, "
			"got (0x%04x), expected (0x%04x)\n", eurus_cmd, ps3_jupiter_eurus_cmd);

	eurus_tag = le16toh(eurus_cmd_hdr->tag);

	if (ps3_jupiter_eurus_tag != eurus_tag)
		fprintf(stderr, "got command IRQ packet with invalid EURUS tag, "
			"got (0x%04x), expected (0x%04x)\n", eurus_tag, ps3_jupiter_eurus_tag);

	memcpy(ps3_jupiter_cmd_xfer_buf, buf, length);

	pthread_mutex_lock(&ps3_jupiter_cmd_lock);
	ps3_jupiter_cmd_status = 0;
	ps3_jupiter_cmd_done = 1;
	pthread_cond_signal(&ps3_jupiter_cmd_cond);
	pthread_mutex_unlock(&ps3_jupiter_cmd_lock);
}

/*
 * ps3_jupiter_irq
 */
static void ps3_jupiter_irq(struct libusb_transfer *transfer)
{
	struct ps3_jupiter_pkt_hdr *pkt_hdr;
	int err;

	switch (transfer->status) {
	case LIBUSB_TRANSFER_COMPLETED:
		if (transfer->actual_length < sizeof(*pkt_hdr)) {
			fprintf(stderr, "got IRQ packet with invalid length (%d)\n",
				transfer->actual_length);
			break;
		}

		pkt_hdr = (struct ps3_jupiter_pkt_hdr *) ps3_jupiter_irq_xfer_buf;

		switch (pkt_hdr->type) {
		case PS3_JUPITER_PKT_CMD:
			ps3_jupiter_cmd_irq(pkt_hdr, transfer->actual_length);
		break;
		case PS3_JUPITER_PKT_EVENT:
			ps3_jupiter_event_irq(pkt_hdr, transfer->actual_length);
		break;
		default:
			fprintf(stderr, "got unknown IRQ packet type (%d)\n",
				pkt_hdr->type);
		}
	break;
	case LIBUSB_TRANSFER_CANCELLED:
	case LIBUSB_TRANSFER_NO_DEVICE:
		return;
	default:
		fprintf(stderr, "IRQ transfer failed (%d)\n", transfer->status);
	}

	libusb_fill_interrupt_transfer(ps3_jupiter_irq_xfer, ps3_jupiter_usb_dev_handle,
		LIBUSB_ENDPOINT_IN | PS3_JUPITER_USB_EP5, ps3_jupiter_irq_xfer_buf,
		sizeof(ps3_jupiter_irq_xfer_buf), ps3_jupiter_irq, NULL, 0);

	err = libusb_submit_transfer(ps3_jupiter_irq_xfer);
	if (err)
		fprintf(stderr, "libusb_submit_transfer failed (%d)\n", err);
}

/*
 * ps3_jupiter_cmd_cb
 */
static void ps3_jupiter_cmd_cb(struct libusb_transfer *transfer)
{
	switch (transfer->status) {
	case LIBUSB_TRANSFER_COMPLETED:
		pthread_mutex_lock(&ps3_jupiter_cmd_lock);
		ps3_jupiter_cmd_xfer_done = 1;
		pthread_cond_signal(&ps3_jupiter_cmd_cond);
		pthread_mutex_unlock(&ps3_jupiter_cmd_lock);
	break;
	default:
		fprintf(stderr, "command transfer failed (%d)\n", transfer->status);
	
		pthread_mutex_lock(&ps3_jupiter_cmd_lock);
		ps3_jupiter_cmd_status = transfer->status;
		ps3_jupiter_cmd_xfer_done = 1;
		ps3_jupiter_cmd_done = 1;
		pthread_cond_signal(&ps3_jupiter_cmd_cond);
		pthread_mutex_unlock(&ps3_jupiter_cmd_lock);
	}
}

/*
 * ps3_jupiter_event_thread_proc
 */
static void *ps3_jupiter_event_thread_proc(void *arg)
{
	int err;

	while (!ps3_jupiter_event_thread_done) {
		err = libusb_handle_events(ps3_jupiter_usb_ctx);
		if (err)
			fprintf(stderr, "libusb_handle_events failed (%d)\n", err);
	}

	return (NULL);
}

/*
 * ps3_jupiter_init
 */
int ps3_jupiter_init(int debug_level)
{
	unsigned char buf[64];
	int err;

	err = libusb_init(&ps3_jupiter_usb_ctx);
	if (err) {
		fprintf(stderr, "libusb_init failed (%d)\n", err);
		return (-1);
	}

	libusb_set_debug(ps3_jupiter_usb_ctx, debug_level);

	ps3_jupiter_usb_dev_handle = libusb_open_device_with_vid_pid(ps3_jupiter_usb_ctx,
		PS3_JUPITER_USB_VENDOR_ID, PS3_JUPITER_USB_PRODUCT_ID);
	if (!ps3_jupiter_usb_dev_handle) {
		fprintf(stderr, "libusb_open_device_with_vid_pid failed\n");
		goto fail_destroy_usb_ctx;
	}

	if (libusb_kernel_driver_active(ps3_jupiter_usb_dev_handle, PS3_JUPITER_USB_IFACE)) {
		fprintf(stdout, "kernel driver is attached, trying to detach it\n");

		err = libusb_detach_kernel_driver(ps3_jupiter_usb_dev_handle,
			PS3_JUPITER_USB_IFACE);
		if (err) {
			fprintf(stderr, "libusb_detach_kernel_driver failed (%d)\n", err);
			goto fail_close_usb_device;
		}

		fprintf(stdout, "kernel driver successfully dettached\n");
	}

	err = libusb_claim_interface(ps3_jupiter_usb_dev_handle, PS3_JUPITER_USB_IFACE);
	if (err) {
		fprintf(stderr, "libusb_claim_interface failed (%d)\n", err);
		goto fail_close_usb_device;
	}

	err = libusb_control_transfer(ps3_jupiter_usb_dev_handle, 0x40, 0x1, 0x9, 0x0,
		ps3_jupiter_devkey, sizeof(ps3_jupiter_devkey), 0);
	if (err < 0) {
		fprintf(stderr, "libusb_control_transfer failed (%d)\n", err);
		goto fail_release_usb_iface;
	}

	err = libusb_control_transfer(ps3_jupiter_usb_dev_handle,
		0xc0, 0x0, 0x2, 0x0, buf, 2, 0);
	if (err < 0) {
		fprintf(stderr, "libusb_control_transfer (%d)\n", err);
		goto fail_release_usb_iface;
	}

	ps3_jupiter_event_listener_head.prev = &ps3_jupiter_event_listener_head;
	ps3_jupiter_event_listener_head.next = &ps3_jupiter_event_listener_head;

	ps3_jupiter_irq_xfer = libusb_alloc_transfer(0);
	if (!ps3_jupiter_irq_xfer) {
		fprintf(stderr, "libusb_alloc_transfer\n");
		goto fail_release_usb_iface;
	}

	libusb_fill_interrupt_transfer(ps3_jupiter_irq_xfer, ps3_jupiter_usb_dev_handle,
		LIBUSB_ENDPOINT_IN | PS3_JUPITER_USB_EP5, ps3_jupiter_irq_xfer_buf,
		sizeof(ps3_jupiter_irq_xfer_buf), ps3_jupiter_irq, NULL, 0);

	err = libusb_submit_transfer(ps3_jupiter_irq_xfer);
	if (err) {
		fprintf(stderr, "libusb_submit_transfer (%d)\n", err);
		goto fail_free_usb_irq_xfer;
	}

	ps3_jupiter_cmd_xfer = libusb_alloc_transfer(0);
	if (!ps3_jupiter_cmd_xfer) {
		fprintf(stderr, "libusb_alloc_transfer\n");
		goto fail_cancel_usb_irq_xfer;
	}

	ps3_jupiter_event_thread_done = 0;

	err = pthread_create(&ps3_jupiter_event_thread, NULL, ps3_jupiter_event_thread_proc,
		NULL);
	if (err) {
		fprintf(stderr, "pthread_create failed (%d)\n", err);
		goto fail_free_usb_cmd_xfer;
	}

	ps3_jupiter_initialized = 1;

	return (0);

fail_free_usb_cmd_xfer:

	libusb_free_transfer(ps3_jupiter_cmd_xfer);

fail_cancel_usb_irq_xfer:

	libusb_cancel_transfer(ps3_jupiter_irq_xfer);

fail_free_usb_irq_xfer:

	libusb_free_transfer(ps3_jupiter_irq_xfer);

fail_release_usb_iface:

	libusb_release_interface(ps3_jupiter_usb_dev_handle, PS3_JUPITER_USB_IFACE);

fail_close_usb_device:

	libusb_close(ps3_jupiter_usb_dev_handle);

fail_destroy_usb_ctx:

	libusb_exit(ps3_jupiter_usb_ctx);

	return (-1);
}

/*
 * ps3_jupiter_fini
 */
void ps3_jupiter_fini(void)
{
	if (!ps3_jupiter_initialized)
		return;

	ps3_jupiter_event_thread_done = 1;

	libusb_cancel_transfer(ps3_jupiter_cmd_xfer);
	libusb_free_transfer(ps3_jupiter_cmd_xfer);

	libusb_cancel_transfer(ps3_jupiter_irq_xfer);
	libusb_free_transfer(ps3_jupiter_irq_xfer);

	libusb_close(ps3_jupiter_usb_dev_handle);
	libusb_exit(ps3_jupiter_usb_ctx);

	ps3_jupiter_initialized = 0;
}

/*
 * ps3_jupiter_register_event_listener
 */
int ps3_jupiter_register_event_listener(struct ps3_jupiter_event_listener *listener)
{
	if (!ps3_jupiter_initialized)
		return (-1);

	pthread_mutex_lock(&ps3_jupiter_event_listener_lock);

	listener->prev = ps3_jupiter_event_listener_head.prev;
	listener->next = ps3_jupiter_event_listener_head.prev->next;
	ps3_jupiter_event_listener_head.prev->next = listener;
	ps3_jupiter_event_listener_head.prev = listener;

	pthread_mutex_unlock(&ps3_jupiter_event_listener_lock);

	return (0);
}

/*
 * ps3_jupiter_unregister_event_listener
 */
int ps3_jupiter_unregister_event_listener(struct ps3_jupiter_event_listener *listener)
{
	if (!ps3_jupiter_initialized)
		return (-1);

	pthread_mutex_lock(&ps3_jupiter_event_listener_lock);

	listener->next->prev = listener->prev;
	listener->prev->next = listener->next;

	pthread_mutex_unlock(&ps3_jupiter_event_listener_lock);

	return (0);
}

/*
 * ps3_jupiter_exec_eurus_cmd
 */
int ps3_jupiter_exec_eurus_cmd(enum ps3_eurus_cmd_id cmd,
	void *payload, unsigned int payload_length,
	unsigned int *response_status,
	unsigned int *response_length, void *response)
{
	struct ps3_jupiter_pkt_hdr *pkt_hdr;
	struct ps3_jupiter_cmd_hdr *cmd_hdr;
	struct ps3_eurus_cmd_hdr *eurus_cmd_hdr;
	uint16_t status;
	int err;

	if (!ps3_jupiter_initialized)
		return (-1);

	pthread_mutex_lock(&ps3_jupiter_cmd_lock);

	if (ps3_jupiter_cmd_busy) {
		pthread_mutex_unlock(&ps3_jupiter_cmd_lock);
		return (-1);
	}

	ps3_jupiter_cmd_busy = 1;

	pthread_mutex_unlock(&ps3_jupiter_cmd_lock);

	pkt_hdr = (struct ps3_jupiter_pkt_hdr *) ps3_jupiter_cmd_xfer_buf;
	memset(pkt_hdr, 0, sizeof(*pkt_hdr));
	pkt_hdr->unknown1 = 1;
	pkt_hdr->unknown2 = 1;
	pkt_hdr->type = PS3_JUPITER_PKT_CMD;

	cmd_hdr = (struct ps3_jupiter_cmd_hdr *) (pkt_hdr + 1);
	memset(cmd_hdr, 0, sizeof(*cmd_hdr));
	ps3_jupiter_cmd_tag++;
	cmd_hdr->unknown1 = 0;
	cmd_hdr->unknown2 = htole16(1);
	cmd_hdr->tag = htole16(ps3_jupiter_cmd_tag);

	eurus_cmd_hdr = (struct ps3_eurus_cmd_hdr *) (cmd_hdr + 1);
	memset(eurus_cmd_hdr, 0, sizeof(*eurus_cmd_hdr));
	ps3_jupiter_eurus_cmd = cmd;
	eurus_cmd_hdr->id = htole16(cmd);
	ps3_jupiter_eurus_tag++;
	eurus_cmd_hdr->tag = htole16(ps3_jupiter_eurus_tag);
	eurus_cmd_hdr->status = htole16(0xa);
	eurus_cmd_hdr->payload_length = htole16(payload_length);

	if (payload_length)
		memcpy(eurus_cmd_hdr + 1, payload, payload_length);

	ps3_jupiter_cmd_xfer_done = 0;
	ps3_jupiter_cmd_done = 0;

	libusb_fill_interrupt_transfer(ps3_jupiter_cmd_xfer, ps3_jupiter_usb_dev_handle,
		LIBUSB_ENDPOINT_OUT | PS3_JUPITER_USB_EP5, ps3_jupiter_cmd_xfer_buf,
		sizeof(struct ps3_jupiter_pkt_hdr) + sizeof(struct ps3_jupiter_cmd_hdr) +
			sizeof(struct ps3_eurus_cmd_hdr) + payload_length,
			ps3_jupiter_cmd_cb, NULL, 0);

	err = libusb_submit_transfer(ps3_jupiter_cmd_xfer);
	if (err) {
		fprintf(stderr, "libusb_submit_transfer failed (%d)\n", err);
		err = -1;
		goto done;
	}

	pthread_mutex_lock(&ps3_jupiter_cmd_lock);
	while (!ps3_jupiter_cmd_xfer_done || !ps3_jupiter_cmd_done)
		pthread_cond_wait(&ps3_jupiter_cmd_cond, &ps3_jupiter_cmd_lock);
	pthread_mutex_unlock(&ps3_jupiter_cmd_lock);

	err = ps3_jupiter_cmd_status;

done:

	if (!err) {
		status = le16toh(eurus_cmd_hdr->status);

		if (response_status)
			*response_status = status;

		if (response_length && response) {
			*response_length = le16toh(eurus_cmd_hdr->payload_length);
			memcpy(response, eurus_cmd_hdr + 1, *response_length);
		}

		if (status != PS3_EURUS_CMD_OK)
			fprintf(stderr, "EURUS command 0x%04x status (0x%04x)\n", cmd, status);

	}

	if (err)
		fprintf(stderr, "EURUS command 0x%04x failed (%d)\n", cmd, err);

	ps3_jupiter_cmd_busy = 0;

	return (err);
}
