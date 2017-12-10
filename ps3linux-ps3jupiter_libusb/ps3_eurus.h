
/*
 * PS3 Eurus
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

#ifndef _PS3_EURUS_H
#define _PS3_EURUS_H

enum ps3_eurus_cmd_id {
	PS3_EURUS_CMD_0x1				= 0x0001,
	PS3_EURUS_CMD_GET_AP_SSID			= 0x0003,
	PS3_EURUS_CMD_SET_AP_SSID			= 0x0005,
	PS3_EURUS_CMD_GET_CHANNEL			= 0x000f,
	PS3_EURUS_CMD_SET_CHANNEL			= 0x0011,
	PS3_EURUS_CMD_SET_ANTENNA			= 0x0029,
	PS3_EURUS_CMD_GET_AP_WEP_CONFIG			= 0x0059,
	PS3_EURUS_CMD_SET_AP_WEP_CONFIG			= 0x005b,
	PS3_EURUS_CMD_0x61				= 0x0061,
	PS3_EURUS_CMD_0x65				= 0x0065,
	PS3_EURUS_CMD_GET_FW_VERSION			= 0x0099,
	PS3_EURUS_CMD_GET_AP_OPMODE			= 0x00b7,
	PS3_EURUS_CMD_SET_AP_OPMODE			= 0x00b9,
	PS3_EURUS_CMD_0xc5				= 0x00c5,
	PS3_EURUS_CMD_GET_AP_WPA_AKM_SUITE		= 0x00c8,
	PS3_EURUS_CMD_SET_AP_WPA_AKM_SUITE		= 0x00c9,
	PS3_EURUS_CMD_GET_AP_WPA_GROUP_CIPHER_SUITE	= 0x00cd,
	PS3_EURUS_CMD_SET_AP_WPA_GROUP_CIPHER_SUITE	= 0x00cf,
	PS3_EURUS_CMD_GET_AP_WPA_PSK_BIN		= 0x00d1,
	PS3_EURUS_CMD_SET_AP_WPA_PSK_BIN		= 0x00d3,
	PS3_EURUS_CMD_0xd5				= 0x00d5,
	PS3_EURUS_CMD_GET_AP_WPA_PSK_PASSPHRASE		= 0x017b,
	PS3_EURUS_CMD_SET_AP_WPA_PSK_PASSPHRASE		= 0x017d,
	PS3_EURUS_CMD_GET_AP_WPA_PAIRWISE_CIPHER_SUITE	= 0x01bd,
	PS3_EURUS_CMD_SET_AP_WPA_PAIRWISE_CIPHER_SUITE	= 0x01bf,
	PS3_EURUS_CMD_0x1d9				= 0x01d9,
	PS3_EURUS_CMD_START_AP				= 0x01dd,
	PS3_EURUS_CMD_0x1ed				= 0x01ed,
	PS3_EURUS_CMD_GET_HW_REVISION			= 0x01fb,
	PS3_EURUS_CMD_0x203				= 0x0203,
	PS3_EURUS_CMD_0x207				= 0x0207,
	PS3_EURUS_CMD_ASSOCIATE				= 0x1001,
	PS3_EURUS_CMD_GET_COMMON_CONFIG			= 0x1003,
	PS3_EURUS_CMD_SET_COMMON_CONFIG			= 0x1005,
	PS3_EURUS_CMD_GET_WEP_CONFIG			= 0x1013,
	PS3_EURUS_CMD_SET_WEP_CONFIG			= 0x1015,
	PS3_EURUS_CMD_GET_WPA_CONFIG			= 0x1017,
	PS3_EURUS_CMD_SET_WPA_CONFIG			= 0x1019,
	PS3_EURUS_CMD_0x1025				= 0x1025,
	PS3_EURUS_CMD_0x1031				= 0x1031,
	PS3_EURUS_CMD_GET_SCAN_RESULTS			= 0x1033,
	PS3_EURUS_CMD_START_SCAN			= 0x1035,
	PS3_EURUS_CMD_DISASSOCIATE			= 0x1037,
	PS3_EURUS_CMD_GET_RSSI				= 0x103d,
	PS3_EURUS_CMD_GET_MAC_ADDR			= 0x103f,
	PS3_EURUS_CMD_SET_MAC_ADDR			= 0x1041,
	PS3_EURUS_CMD_0x104d				= 0x104d,
	PS3_EURUS_CMD_0x104f				= 0x104f,
	PS3_EURUS_CMD_0x1051				= 0x1051,
	PS3_EURUS_CMD_0x105f				= 0x105f,
	PS3_EURUS_CMD_0x1109				= 0x1109,
	PS3_EURUS_CMD_0x110b				= 0x110b,
	PS3_EURUS_CMD_0x110d				= 0x110d,
	PS3_EURUS_CMD_0x1133				= 0x1133,
	PS3_EURUS_CMD_0x114b				= 0x114b,
	PS3_EURUS_CMD_0x114f				= 0x114f,
	PS3_EURUS_CMD_0x115b				= 0x115b,
	PS3_EURUS_CMD_0x115d				= 0x115d,
	PS3_EURUS_CMD_0x115f				= 0x115f,
	PS3_EURUS_CMD_SET_MCAST_ADDR_FILTER		= 0x1161,
	PS3_EURUS_CMD_CLEAR_MCAST_ADDR_FILTER		= 0x1163,
	PS3_EURUS_CMD_GET_MCAST_ADDR_FILTER		= 0x1165,
	PS3_EURUS_CMD_0x116d				= 0x116d,
	PS3_EURUS_CMD_0x116f				= 0x116f,
	PS3_EURUS_CMD_GET_MAC_ADDR_LIST			= 0x1117,
	PS3_EURUS_CMD_0x1171				= 0x1171,
	PS3_EURUS_CMD_GET_CHANNEL_INFO			= 0xfffd,
};

enum ps3_eurus_cmd_status {
	PS3_EURUS_CMD_OK				= 0x0001,
	PS3_EURUS_CMD_INVALID_LENGTH			= 0x0002,
	PS3_EURUS_CMD_UNSUPPORTED			= 0x0003,
	PS3_EURUS_CMD_INVALID_PARAMETER			= 0x0004,
};

enum ps3_eurus_event_type {
	PS3_EURUS_EVENT_TYPE_0x40			= 0x00000040,
	PS3_EURUS_EVENT_TYPE_0x80			= 0x00000080,
	PS3_EURUS_EVENT_TYPE_0x400			= 0x00000400,
	PS3_EURUS_EVENT_TYPE_0x80000000			= 0x80000000
};

enum ps3_eurus_event_id {
	/* event type 0x00000040 */

	PS3_EURUS_EVENT_DEAUTH				= 0x00000001,

	/* event type 0x00000080 */

	PS3_EURUS_EVENT_BEACON_LOST			= 0x00000001,
	PS3_EURUS_EVENT_CONNECTED			= 0x00000002,
	PS3_EURUS_EVENT_SCAN_COMPLETED			= 0x00000004,
	PS3_EURUS_EVENT_WPA_CONNECTED			= 0x00000020,
	PS3_EURUS_EVENT_WPA_ERROR			= 0x00000040,

	/* event type 0x80000000 */

	PS3_EURUS_EVENT_DEVICE_READY			= 0x00000001,
};

enum ps3_eurus_ap_opmode {
	PS3_EURUS_AP_OPMODE_11B				= 0x00000000,
	PS3_EURUS_AP_OPMODE_11G				= 0x00000001,
	PS3_EURUS_AP_OPMODE_11BG			= 0x00000002,
};

enum ps3_eurus_bss_type {
	PS3_EURUS_BSS_INFRA				= 0x00,
	PS3_EURUS_BSS_ADHOC				= 0x02,
};

enum ps3_eurus_auth_mode {
	PS3_EURUS_AUTH_OPEN				= 0x00,
	PS3_EURUS_AUTH_SHARED_KEY			= 0x01,
};

enum ps3_eurus_opmode {
	PS3_EURUS_OPMODE_11BG				= 0x00,
	PS3_EURUS_OPMODE_11B				= 0x01,
	PS3_EURUS_OPMODE_11G				= 0x02,
};

enum ps3_eurus_preamble_mode {
	PS3_EURUS_PREAMBLE_SHORT			= 0x00,
	PS3_EURUS_PREAMBLE_LONG				= 0x01,
};

enum ps3_eurus_wep_security_mode {
	PS3_EURUS_WEP_SECURITY_NONE			= 0x00,
	PS3_EURUS_WEP_SECURITY_40BIT			= 0x01,
	PS3_EURUS_WEP_SECURITY_104BIT			= 0x02,
};

enum ps3_eurus_wpa_security_mode {
	PS3_EURUS_WPA_SECURITY_WPA			= 0x00,
	PS3_EURUS_WPA_SECURITY_WPA2			= 0x01,
};

enum ps3_eurus_wpa_psk_type {
	PS3_EURUS_WPA_PSK_PASSPHRASE			= 0x00,
	PS3_EURUS_WPA_PSK_BIN				= 0x01,
};

enum ps3_eurus_wpa_cipher_suite {
	PS3_EURUS_WPA_CIPHER_SUITE_WPA_TKIP		= 0x0050f202,
	PS3_EURUS_WPA_CIPHER_SUITE_WPA_AES		= 0x0050f204,
	PS3_EURUS_WPA_CIPHER_SUITE_WPA2_TKIP		= 0x000fac02,
	PS3_EURUS_WPA_CIPHER_SUITE_WPA2_AES		= 0x000fac04,
};

enum ps3_eurus_wpa_akm_suite {
	PS3_EURUS_WPA_AKM_SUITE_WPA_PSK			= 0x0050f202,
	PS3_EURUS_WPA_AKM_SUITE_WPA2_PSK		= 0x000fac02,
};

struct ps3_eurus_cmd_hdr {
	uint16_t id;			/* enum ps3_eurus_cmd_id */
	uint16_t tag;
	uint16_t status;			/* enum ps3_eurus_cmd_status */
	uint16_t payload_length;
	uint8_t res[4];
} __attribute__ ((packed));

struct ps3_eurus_cmd_0x1 {
	uint8_t unknown;
	uint8_t res[3];
} __attribute__ ((packed));

struct ps3_eurus_cmd_ap_ssid {
	uint8_t ssid[32];
	uint8_t res[4];
} __attribute__ ((packed));

struct ps3_eurus_cmd_get_channel {
	uint8_t unknown[35];
	uint16_t channel;
} __attribute__ ((packed));

struct ps3_eurus_cmd_set_channel {
	uint8_t channel;
} __attribute__ ((packed));

struct ps3_eurus_cmd_set_antenna {
	uint8_t unknown1;
	uint8_t unknown2;
} __attribute__ ((packed));

struct ps3_eurus_cmd_ap_wep_config {
	uint8_t unknown1;
	uint8_t unknown2;
	uint8_t security_mode;	/* enum ps3_eurus_wep_security_mode */
	uint8_t unknown3;
	uint8_t key[4][18];
} __attribute__ ((packed));

struct ps3_eurus_cmd_0x61 {
	uint8_t unknown;
} __attribute__ ((packed));

struct ps3_eurus_cmd_0x65 {
	uint8_t unknown;
} __attribute__ ((packed));

struct ps3_eurus_cmd_get_fw_version {
	uint8_t version[62];	/* string */
} __attribute__ ((packed));

struct ps3_eurus_cmd_ap_opmode {
	uint32_t opmode;	/* enum ps3_eurus_ap_opmode */
} __attribute__ ((packed));

struct ps3_eurus_cmd_0xc5 {
	uint32_t unknown;
} __attribute__ ((packed));

struct ps3_eurus_cmd_ap_wpa_akm_suite {
	uint32_t suite;	/* enum ps3_eurus_wpa_akm_suite */
	uint8_t unknown;
} __attribute__ ((packed));

struct ps3_eurus_cmd_ap_wpa_group_cipher_suite {
	uint32_t cipher_suite;	/* enum ps3_eurus_wpa_cipher_suite */
} __attribute__ ((packed));

struct ps3_eurus_cmd_ap_wpa_psk_bin {
	uint8_t psk[64];
} __attribute__ ((packed));

struct ps3_eurus_cmd_0xd5 {
	uint16_t unknown;
	uint8_t res[2];
} __attribute__ ((packed));

struct ps3_eurus_cmd_ap_wpa_psk_passphrase {
	uint8_t unknown;
	uint8_t passphrase[32];
} __attribute__ ((packed));

struct ps3_eurus_cmd_ap_wpa_pairwise_cipher_suite {
	uint32_t cipher_suite;	/* enum ps3_eurus_wpa_cipher_suite */
	uint8_t unknown;
} __attribute__ ((packed));

struct ps3_eurus_cmd_0x1d9 {
	uint8_t unknown1;
	uint8_t unknown2;
	uint8_t res[2];
} __attribute__ ((packed));

struct ps3_eurus_cmd_start_ap {
	uint8_t unknown;
} __attribute__ ((packed));

struct ps3_eurus_cmd_0x1ed {
	uint32_t unknown1;
	uint8_t unknown2;
	uint8_t unknown3;
	uint8_t unknown4;
	uint8_t unknown5;
	uint8_t unknown6;
	uint8_t unknown7;
	uint8_t unknown8;
} __attribute__ ((packed));

struct ps3_eurus_cmd_get_hw_revision {
	uint8_t unknown[4];
} __attribute__ ((packed));

struct ps3_eurus_cmd_0x203 {
	uint32_t unknown;
} __attribute__ ((packed));

struct ps3_eurus_cmd_0x207 {
	uint32_t unknown;
} __attribute__ ((packed));

struct ps3_eurus_cmd_associate {
	uint8_t unknown;
} __attribute__ ((packed));

struct ps3_eurus_cmd_common_config {
	uint8_t bss_type;		/* enum ps3_eurus_bss_type */
	uint8_t auth_mode;		/* enum ps3_eurus_auth_mode */
	uint8_t opmode;		/* enum ps3_eurus_opmode */
	uint8_t unknown;
	uint8_t bssid[6];
	uint16_t capability;
	uint8_t ie[0];
} __attribute__ ((packed));

struct ps3_eurus_cmd_wep_config {
	uint8_t unknown1;
	uint8_t security_mode;		/* enum ps3_eurus_wep_security_mode */
	uint16_t unknown2;
	uint8_t key[4][16];
} __attribute__ ((packed));

struct ps3_eurus_cmd_wpa_config {
	uint8_t unknown;
	uint8_t security_mode;		/* enum ps3_eurus_wpa_security_mode */
	uint8_t psk_type;			/* enum ps3_eurus_wpa_psk_type */
	uint8_t psk[64];
	uint32_t group_cipher_suite;	/* enum ps3_eurus_wpa_cipher_suite */
	uint32_t pairwise_cipher_suite;	/* enum ps3_eurus_wpa_cipher_suite */
	uint32_t akm_suite;		/* enum ps3_eurus_wpa_akm_suite */
} __attribute__ ((packed));

struct ps3_eurus_cmd_0x1025 {
	uint8_t preamble_mode;	/* enum ps3_eurus_preamble_mode */
	uint8_t res[3];
} __attribute__ ((packed));

struct ps3_eurus_cmd_0x1031 {
	uint8_t unknown1;
	uint8_t unknown2;
} __attribute__ ((packed));

struct ps3_eurus_scan_result {
	uint16_t length;
	uint8_t bssid[6];
	uint8_t rssi;
	uint64_t timestamp;
	uint16_t beacon_period;		/* in msec */
	uint16_t capability;
	uint8_t ie[0];
} __attribute__ ((packed));

#define PS3_EURUS_SCAN_RESULTS_MAXSIZE	0x5b0

struct ps3_eurus_cmd_get_scan_results {
	uint8_t count;
	struct ps3_eurus_scan_result result[0];
} __attribute__ ((packed));

struct ps3_eurus_cmd_start_scan {
	uint8_t unknown1;
	uint8_t unknown2;
	uint16_t channel_dwell;	/* in msec */
	uint8_t res[6];
	uint8_t ie[0];
} __attribute__ ((packed));

struct ps3_eurus_cmd_disassociate {
	uint8_t unknown;
} __attribute__ ((packed));

struct ps3_eurus_cmd_get_rssi {
	uint8_t res[10];
	uint8_t rssi;
} __attribute__ ((packed));

struct ps3_eurus_cmd_get_mac_addr {
	uint8_t unknown;
	uint8_t mac_addr[6];
} __attribute__ ((packed));

struct ps3_eurus_cmd_set_mac_addr {
	uint8_t mac_addr[6];
} __attribute__ ((packed));

struct ps3_eurus_cmd_0x104d {
	uint8_t unknown;
} __attribute__ ((packed));

struct ps3_eurus_cmd_0x104f {
	uint8_t unknown;
} __attribute__ ((packed));

struct ps3_eurus_cmd_0x1051 {
	uint8_t res[2];
	uint8_t unknown1;
	uint8_t unknown2;
} __attribute__ ((packed));

struct ps3_eurus_cmd_0x105f {
	uint16_t channel_info;
	uint8_t mac_addr[6];
	uint8_t unknown1;
	uint8_t unknown2;
} __attribute__ ((packed));

struct ps3_eurus_cmd_0x1109 {
	uint16_t unknown1;
	uint16_t unknown2;
	uint16_t unknown3;
	uint16_t unknown4;
	uint16_t unknown5;
	uint16_t unknown6;
	uint16_t unknown7;
	uint8_t unknown8;
	uint8_t res;
	uint8_t unknown9;
	uint8_t unknown10;
	uint16_t unknown11;
	uint16_t unknown12;
} __attribute__ ((packed));

struct ps3_eurus_cmd_0x110b {
	uint32_t unknown1;
	uint8_t res[4];
	uint32_t unknown2;
} __attribute__ ((packed));

struct ps3_eurus_cmd_0x110d {
	uint8_t res1[12];
	uint32_t unknown1;
	uint32_t unknown2;
	uint32_t unknown3;
	uint32_t unknown4;
	uint32_t unknown5;
	uint32_t unknown6;
	uint32_t unknown7;
	uint8_t res2[88];
} __attribute__ ((packed));

struct ps3_eurus_cmd_0x1133 {
	uint8_t unknown1;
	uint8_t unknown2;
	uint8_t unknown3;
	uint8_t unknown4;
	uint32_t unknown5;
	uint8_t res[6];
} __attribute__ ((packed));

struct ps3_eurus_cmd_0x114f {
	uint8_t res[1304];
} __attribute__ ((packed));

struct ps3_eurus_cmd_0x115b {
	uint16_t unknown1;
	uint16_t unknown2;
	uint8_t mac_addr[6];
	uint8_t res[84];
} __attribute__ ((packed));

struct ps3_eurus_cmd_mcast_addr_filter {
	uint32_t word[8];
} __attribute__ ((packed));

struct ps3_eurus_cmd_0x116d {
	uint32_t unknown;
} __attribute__ ((packed));

struct ps3_eurus_cmd_0x116f {
	uint32_t unknown;
} __attribute__ ((packed));

#define PS3_EURUS_MAC_ADDR_LIST_MAXSIZE	0xc2

struct ps3_eurus_cmd_get_mac_addr_list {
	uint16_t count;	/* number of MAC addresses */
	uint8_t mac_addr[0];
} __attribute__ ((packed));

struct ps3_eurus_cmd_get_channel_info {
	uint16_t channel_info;
} __attribute__ ((packed));

struct ps3_eurus_event_hdr {
	uint32_t type;			/* enum ps3_eurus_event_type */
	uint32_t id;			/* enum ps3_eurus_event_id */
	uint32_t timestamp;
	uint32_t payload_length;
	uint32_t unknown;
} __attribute__ ((packed));

struct ps3_eurus_event {
	struct ps3_eurus_event_hdr hdr;
	uint8_t payload[44];
} __attribute__ ((packed));

/*
 * ps3_eurus_rssi2percentage
 */
static inline uint8_t ps3_eurus_rssi2percentage(uint8_t rssi)
{
	if (rssi > 89)
		return 1;
	else if (rssi < 50)
		return 100;
	else
		return ((90 - rssi) * 100) / 40;
}

#define PS3_EURUS_MCAST_ADDR_HASH2VAL(h)	(1 << ((h) & 0x1f))
#define PS3_EURUS_MCAST_ADDR_HASH2POS(h)	(((h) >> 5) & 0x7)

/*
 * ps3_eurus_mcast_addr_hash
 */
static inline uint8_t ps3_eurus_mcast_addr_hash(const uint8_t mac_addr[ETH_ALEN])
{
	uint8_t buf[ETH_ALEN];
	uint32_t h;
	unsigned int i, j;

	memcpy(buf, mac_addr, ETH_ALEN);

	/* reverse bits in each byte */

	for (i = 0; i < ETH_ALEN; i++) {
		buf[i] = (buf[i] >> 4) | ((buf[i] & 0xf) << 4);
		buf[i] = ((buf[i] & 0xcc) >> 2) | ((buf[i] & 0x33) << 2);
		buf[i] = ((buf[i] & 0xaa) >> 1) | ((buf[i] & 0x55) << 1);
	}

        h = 0xffffffff;

        for (i = 0; i < ETH_ALEN; i++) {
                h = (((unsigned int) buf[i]) << 24) ^ h;

                for (j = 0; j < 8; j++) {
                        if (((int) h) >= 0) {
                                h = h << 1;
                        } else {
                                h = (h << 1) ^ 0x4c10000;
                                h = h ^ 0x1db7;
                        }
                }
        }

        h = ((h >> 24) & 0xf8) | (h & 0x7);

        return (h & 0xff);
}

/*
 * ps3_eurus_make_cmd_0x1109
 */
static inline void ps3_eurus_make_cmd_0x1109(struct ps3_eurus_cmd_0x1109 *cmd_0x1109,
	uint8_t arg1, uint16_t arg2, uint16_t arg3, uint16_t arg4, uint16_t arg5)
{
	memset(cmd_0x1109, 0, sizeof(*cmd_0x1109));

	cmd_0x1109->unknown1 = htole16(0x1);
	cmd_0x1109->unknown8 = arg1;

	if (arg1 == 0x0) {
		cmd_0x1109->unknown6 = htole16(0xa);
	} else if (arg1 == 0x1) {
		cmd_0x1109->unknown2 = htole16(arg2);
		cmd_0x1109->unknown3 = htole16(arg3);
		cmd_0x1109->unknown4 = htole16(arg5);

		if (arg2 == 0x0)
			cmd_0x1109->unknown5 = htole16(0x6);
		else
			cmd_0x1109->unknown5 = htole16(0x2);

		cmd_0x1109->unknown7 = htole16(arg4);
		cmd_0x1109->unknown9 = 0xff;
		cmd_0x1109->unknown10 = 0xff;
		cmd_0x1109->unknown11 = 0xffff;
		cmd_0x1109->unknown12 = 0xffff;
	}
}

#endif
