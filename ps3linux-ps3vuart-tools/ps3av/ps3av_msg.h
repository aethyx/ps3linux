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

#ifndef _PS3AV_MSG_H
#define _PS3AV_MSG_H

enum ps3av_command {
	PS3AV_CMD_AV_INIT		= 0x00000001,
	PS3AV_CMD_AV_FINI		= 0x00000002,
	PS3AV_CMD_AV_GET_HW_CONF	= 0x00000003,
	PS3AV_CMD_AV_GET_MONITOR_INFO	= 0x00000004,
	PS3AV_CMD_AV_TV_MUTE		= 0x0000000a,
	PS3AV_CMD_AV_GET_HDCP_KSV	= 0x0000000c,
	PS3AV_CMD_AV_VIDEO_MUTE		= 0x00010002,

	PS3AV_CMD_VIDEO_INIT		= 0x01000001,
	PS3AV_CMD_VIDEO_PITCH		= 0x01000005,

	PS3AV_CMD_AUDIO_INIT		= 0x02000001,
	PS3AV_CMD_AUDIO_MUTE		= 0x02000003,

	PS3AV_CMD_REPLY			= 0x80000000,
};

enum ps3av_status {
	PS3AV_STATUS_SUCCESS			= 0,
	PS3AV_STATUS_VUART_RX_ERR		= 1,
	PS3AV_STATUS_SC_COMM_ERR		= 2,
	PS3AV_STATUS_INVALID_CMD		= 3,
	PS3AV_STATUS_INVALID_PORT		= 4,
	PS3AV_STATUS_FAILURE			= 11,
	PS3AV_STATUS_UNSUPPORTED_CMD		= 12,
	PS3AV_STATUS_INVALID_VIDEO_PARAM	= 14,
	PS3AV_STATUS_INVALID_AV_PARAM		= 16,
	PS3AV_STATUS_INVALID_AUDIO_PARAM	= 17,
};

enum ps3av_port {
	PS3AV_PORT_HDMI_0	= 0x0000,
	PS3AV_PORT_HDMI_1	= 0x0001,
	PS3AV_PORT_AVMULTI_0	= 0x0010,
	PS3AV_PORT_SPDIF_0	= 0x0020,
	PS3AV_PORT_SPDIF_1	= 0x0021,
};

enum ps3av_head {
	PS3AV_HEAD_A	= 0,
	PS3AV_HEAD_B	= 1,
};

enum ps3av_monitor_type {
	PS3AV_MONITOR_TYPE_HDMI	= 1,
	PS3AV_MONITOR_TYPE_DVI	= 2,
};

enum ps3av_mute {
	PS3AV_MUTE_OFF	= 0x0000,
	PS3AV_MUTE_ON	= 0x0001,
};

struct ps3av_resolution {
	uint32_t bits;
	uint32_t native;
};

struct ps3av_cs {
	uint8_t rgb;
	uint8_t yuv444;
	uint8_t yuv422;
	uint8_t res;
};

struct ps3av_color {
	uint16_t red_x;
	uint16_t red_y;
	uint16_t green_x;
	uint16_t green_y;
	uint16_t blue_x;
	uint16_t blue_y;
	uint16_t white_x;
	uint16_t white_y;
	uint32_t gamma;
};

struct ps3av_audio {
	uint8_t type;
	uint8_t nr_chan;
	uint8_t fs;
	uint8_t sbit;
};

struct ps3av_monitor {
	uint8_t port;				/* enum ps3av_port */
	uint8_t id[10];
	uint8_t type;				/* enum ps3av_monitor_type */
	uint8_t name[16];
	struct ps3av_resolution res_60;
	struct ps3av_resolution res_50;
	struct ps3av_resolution res_other;
	struct ps3av_resolution res_vesa;
	struct ps3av_cs cs;
	struct ps3av_color color;
	uint8_t ai;
	uint8_t speaker;
	uint8_t nr_audio;
	struct ps3av_audio audio[0];
	uint8_t res[169];
} __attribute__ ((packed));

#define PS3AV_VERSION	0x205

struct ps3av_header {
	uint16_t version;
	uint16_t length;
};

#define PS3AV_HDR(_p)	((struct ps3av_header *) (_p))

struct ps3av_request {
	struct ps3av_header hdr;
	uint32_t cmd;			/* enum ps3av_command */
};

#define PS3AV_REQ(_p)	((struct ps3av_request *) (_p))

struct ps3av_reply {
	struct ps3av_header hdr;
	uint32_t cmd;			/* enum ps3av_command */
	uint32_t status;		/* enum ps3av_status */
};

#define PS3AV_REPLY(_p)	((struct ps3av_reply *) (_p))

struct ps3av_av_init {
	struct ps3av_request req;
	uint32_t event_bit;
};

struct ps3av_av_get_hw_conf_reply {
	struct ps3av_reply reply;
	uint16_t nr_hdmi;
	uint16_t nr_avmulti;
	uint16_t nr_spdif;
	uint8_t res[2];
};

struct ps3av_av_get_monitor_info {
	struct ps3av_request req;
	uint16_t port;			/* enum ps3av_port */
	uint8_t res[2];
};

struct ps3av_av_get_monitor_info_reply {
	struct ps3av_reply reply;
	struct ps3av_monitor monitor;
};

struct ps3av_av_tv_mute {
	struct ps3av_request req;
	uint16_t port;			/* enum ps3av_port */
	uint16_t mute;			/* enum ps3av_mute */
};

struct ps3av_av_get_hdcp_ksv_reply {
	struct ps3av_reply reply;
	uint32_t ksv_length;
	uint8_t ksv[12];
};

struct ps3av_av_video_mute {
	struct ps3av_request req;
	uint16_t port;			/* enum ps3av_port */
	uint16_t mute;			/* enum ps3av_mute */
};

struct ps3av_video_pitch {
	struct ps3av_request req;
	uint32_t head;			/* enum ps3av_head */
	uint32_t pitch;
};

struct ps3av_audio_mute {
	struct ps3av_request req;
	uint8_t port;			/* enum ps3av_port */
	uint8_t res[3];
	uint32_t mute;			/* enum ps3av_mute */
};

static inline void
ps3av_init_request(struct ps3av_request *req, uint16_t length, uint32_t cmd)
{
	PS3AV_HDR(req)->version = PS3AV_VERSION;
	PS3AV_HDR(req)->length = length;
	req->cmd = cmd;
}

#endif /* _PS3AV_MSG_H */
