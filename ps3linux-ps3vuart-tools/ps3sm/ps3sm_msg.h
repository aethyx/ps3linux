/*-
 * Copyright (C) 2011, 2012 glevand <geoffrey.levand@mail.ru>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
 *    without modification, immediately at the beginning of the file.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _PS3SM_MSG_H
#define _PS3SM_MSG_H

enum ps3sm_service {
	PS3SM_SID_REQ			= 0x0001,
	PS3SM_SID_EXT_EVENT		= 0x0004,
	PS3SM_SID_SET_NEXT_OP		= 0x0005,
	PS3SM_SID_REQ_ERROR		= 0x0006,
	PS3SM_SID_SET_ATTR		= 0x0008,
	PS3SM_SID_GET_INTER_LPAR_PARAM	= 0x0009,
	PS3SM_SID_SET_INTER_LPAR_PARAM	= 0x000a,
	PS3SM_SID_CTL_LED		= 0x000c,
	PS3SM_SID_RING_BUZZER		= 0x0015,
};

enum ps3sm_request_type {
	PS3SM_REQ_TYPE_SHUTDOWN	= 1,
};

enum ps3sm_guest_os_id {
	PS3SM_GOS_ID_SELF	= 0,
};

enum ps3sm_external_event {
	PS3SM_EXT_EVENT_POWER_PRESSED	= 3,
	PS3SM_EXT_EVENT_POWER_RELEASED	= 4,
	PS3SM_EXT_EVENT_RESET_PRESSED	= 5,
	PS3SM_EXT_EVENT_RESET_RELEASED	= 6,
	PS3SM_EXT_EVENT_THERMAL_ALERT	= 7,
	PS3SM_EXT_EVENT_THERMAL_CLEARED	= 8,
};

enum ps3sm_next_operation {
	PS3SM_NEXT_OP_SYS_SHUTDOWN	= 1,
	PS3SM_NEXT_OP_SYS_REBOOT	= 2,
	PS3SM_NEXT_OP_LPAR_REBOOT	= 0x82,
};

enum ps3sm_wake_source {
	PS3SM_WAKE_SRC_DEFAULT	= 0,
	PS3SM_WAKE_SRC_WOL	= 0x00000400,
	PS3SM_WAKE_SRC_POR	= 0x80000000,
};

enum ps3sm_attr {
	PS3SM_ATTR_POWER	= (1 << 0),
	PS3SM_ATTR_RESET	= (1 << 1),
	PS3SM_ATTR_THERMAL	= (1 << 2),
};

#define PS3SM_VERSION	1

struct ps3sm_header {
	uint8_t version;
	uint8_t length;
	uint8_t res1[2];
	uint32_t payload_length;
	uint16_t sid;			/* enum ps3sm_service */
	uint8_t res2[2];
	uint32_t tag;
};

#define PS3SM_HDR(_p)	((struct ps3sm_header *) (_p))

#define PS3SM_REQ_VERSION	1

struct ps3sm_req {
	struct ps3sm_header hdr;
	uint8_t version;
	uint8_t type;			/* enum ps3sm_request_type */
	uint8_t gos_id;			/* enum ps3sm_guest_os_id */
	uint8_t res[13];
};

#define PS3SM_EXT_EVENT_VERSION	1

struct ps3sm_ext_event {
	struct ps3sm_header hdr;
	uint8_t version;
	uint8_t type;			/* enum ps3sm_external_event */
	uint8_t res1[2];
	uint32_t value;
	uint8_t res2[8];
};

#define PS3SM_SET_NEXT_OP_VERSION	3

struct ps3sm_set_next_op {
	struct ps3sm_header hdr;
	uint8_t version;
	uint8_t type;			/* enum ps3sm_next_operation */
	uint8_t gos_id;
	uint8_t res1;
	uint8_t wake_src;		/* enum ps3sm_wake_source */
	uint8_t res2[8];
};

#define PS3SM_REQ_ERROR_VERSION	1

struct ps3sm_req_error {
	struct ps3sm_header hdr;
	uint8_t version;
	uint8_t res[7];
};

#define PS3SM_SET_ATTR_VERSION	1

struct ps3sm_set_attr {
	struct ps3sm_header hdr;
	uint8_t version;
	uint8_t res[3];
	uint32_t attrs;			/* enum ps3sm_attr */
};

#define PS3SM_GET_INTER_LPAR_PARAM_VERSION	1

struct ps3sm_get_inter_lpar_param {
	struct ps3sm_header hdr;
	uint8_t version;
	uint8_t arg;
	uint8_t res[3];
};

struct ps3sm_get_inter_lpar_param_reply {
	struct ps3sm_header hdr;
	union {
		struct {
			uint8_t data[1536];
		} arg1;
		struct {
			uint8_t val1;
			uint8_t val2;
			uint8_t val3;
			uint8_t val4;
			uint32_t val5;
			uint64_t val6;
		} arg2;
	} u;
};

#define PS3SM_CTL_LED_VERSION	1

struct ps3sm_ctl_led {
	struct ps3sm_header hdr;
	uint8_t version;
	uint8_t arg1;
	uint8_t arg2;
	uint8_t res1;
	uint8_t arg3;
	uint8_t arg4;
	uint8_t res2[2];
};

#define PS3SM_RING_BUZZER_VERSION	1

struct ps3sm_ring_buzzer {
	struct ps3sm_header hdr;
	uint8_t version;
	uint8_t arg1;
	uint8_t arg2;
	uint8_t res;
	uint32_t arg3;
};

static inline void
ps3sm_init_header(struct ps3sm_header *hdr, uint32_t payload_length,
    uint16_t sid, uint32_t tag)
{
	hdr->version = PS3SM_VERSION;
	hdr->length = sizeof(struct ps3sm_header);
	hdr->payload_length = payload_length;
	hdr->sid = sid;
	hdr->tag = tag;
}

#endif /* _PS3SM_MSG_H */
