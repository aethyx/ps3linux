
/*
 * PS3 Jupiter AP
 *
 * Copyright (C) 2012-2013 glevand <geoffrey.levand@mail.ru>
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/interrupt.h>
#include <linux/notifier.h>
#include <linux/ctype.h>

#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/if_arp.h>
#include <linux/ieee80211.h>
#include <net/iw_handler.h>

#include "ps3_eurus.h"
#include "ps3_jupiter.h"

#define PS3_JUPITER_AP_CMD_BUFSIZE		2048

#define PS3_JUPITER_AP_IFACE			0x5
#define PS3_JUPITER_AP_EP			0x7

#define PS3_JUPITER_AP_WEP_KEYS			4

#define PS3_JUPITER_AP_WPA_PSK_LENGTH		32

#define PS3_JUPITER_AP_RX_URBS			4
#define PS3_JUPITER_AP_RX_BUFSIZE		0x620

#define PS3_JUPITER_AP_TX_URBS			16

enum ps3_jupiter_ap_status {
	PS3_JUPITER_AP_READY = 0,
};

enum ps3_jupiter_ap_config_bits {
	PS3_JUPITER_AP_CONFIG_ESSID_SET = 0,
	PS3_JUPITER_AP_CONFIG_CHANNEL_SET,
	PS3_JUPITER_AP_CONFIG_WPA_PSK_SET
};

enum ps3_jupiter_ap_start_ap_status {
	PS3_JUPITER_AP_STOPPED = 0,
	PS3_JUPITER_AP_START_IN_PROGRESS,
	PS3_JUPITER_AP_STARTED
};

enum ps3_jupiter_ap_opmode {
	PS3_JUPITER_AP_OPMODE_11B = 0,
	PS3_JUPITER_AP_OPMODE_11G,
	PS3_JUPITER_AP_OPMODE_11BG
};

enum ps3_jupiter_ap_auth_mode {
	PS3_JUPITER_AP_AUTH_OPEN = 0,
	PS3_JUPITER_AP_AUTH_SHARED_KEY
};

enum ps3_jupiter_ap_wpa_mode {
	PS3_JUPITER_AP_WPA_MODE_NONE = 0,
	PS3_JUPITER_AP_WPA_MODE_WPA,
	PS3_JUPITER_AP_WPA_MODE_WPA2
};

enum ps3_jupiter_ap_cipher_mode {
	PS3_JUPITER_AP_CIPHER_NONE = 0,
	PS3_JUPITER_AP_CIPHER_WEP,
	PS3_JUPITER_AP_CIPHER_TKIP,
	PS3_JUPITER_AP_CIPHER_AES
};

struct ps3_jupiter_ap_dev {
	struct net_device *netdev;

	struct usb_device *udev;

	spinlock_t lock;

	unsigned long status;

	struct iw_public_data wireless_data;
	struct iw_statistics wireless_stat;

	struct notifier_block event_listener;

	u16 channel_info;

	unsigned long config_status;

	enum ps3_jupiter_ap_opmode opmode;

	enum ps3_jupiter_ap_auth_mode auth_mode;

	enum ps3_jupiter_ap_wpa_mode wpa_mode;
	enum ps3_jupiter_ap_cipher_mode group_cipher_mode;
	enum ps3_jupiter_ap_cipher_mode pairwise_cipher_mode;

	u8 essid[IW_ESSID_MAX_SIZE];
	unsigned int essid_length;

	u8 channel;

	unsigned long key_config_status;
	u8 key[PS3_JUPITER_AP_WEP_KEYS][IW_ENCODING_TOKEN_MAX];
	unsigned int key_length[PS3_JUPITER_AP_WEP_KEYS];
	unsigned int curr_key_index;

	u8 psk[PS3_JUPITER_AP_WPA_PSK_LENGTH * 2];
	unsigned int psk_length;
	int psk_passphrase;

	struct mutex start_ap_lock;
	struct workqueue_struct *start_ap_queue;
	struct delayed_work start_ap_work;
	enum ps3_jupiter_ap_start_ap_status start_ap_status;
	struct completion start_ap_done_comp;

	struct usb_anchor rx_urb_anchor;
	struct sk_buff_head rx_skb_queue;
	struct tasklet_struct rx_tasklet;

	struct usb_anchor tx_urb_anchor;
	atomic_t tx_submitted_urbs;
};

static int max_txurbs = PS3_JUPITER_AP_TX_URBS;
module_param(max_txurbs, int, S_IRUGO);
MODULE_PARM_DESC(max_txurbs, "Maximum number of Tx URBs");

static const int ps3_jupiter_ap_channel_freq[] = {
	2412,
	2417,
	2422,
	2427,
	2432,
	2437,
	2442,
	2447,
	2452,
	2457,
	2462,
	2467,
	2472,
	2484
};

static const int ps3_jupiter_ap_bitrate[] = {
	1000000,
	2000000,
	5500000,
	11000000,
	6000000,
	9000000,
	12000000,
	18000000,
	24000000,
	36000000,
	48000000,
	54000000
};

static int ps3_jupiter_ap_prepare_rx_urb(struct ps3_jupiter_ap_dev *japd,
	struct urb *urb);

static void ps3_jupiter_ap_purge_rx_skb_queue(struct ps3_jupiter_ap_dev *japd);

static int ps3_jupiter_ap_tx_skb(struct ps3_jupiter_ap_dev *japd, struct sk_buff *skb);

static void ps3_jupiter_ap_free_tx_urbs(struct ps3_jupiter_ap_dev *japd);

static void ps3_jupiter_ap_start_ap(struct ps3_jupiter_ap_dev *japd);

static int ps3_jupiter_ap_stop_ap(struct ps3_jupiter_ap_dev *japd);

static void ps3_jupiter_ap_reset_state(struct ps3_jupiter_ap_dev *japd);

/*
 * ps3_jupiter_ap_freq_to_channel
 */
static u8 ps3_jupiter_ap_freq_to_channel(u32 freq)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ps3_jupiter_ap_channel_freq); i++) {
		if (ps3_jupiter_ap_channel_freq[i] == freq)
			return (i + 1);
	}

	return 0;
}

/*
 * ps3_jupiter_ap_get_name
 */
static int ps3_jupiter_ap_get_name(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	strcpy(wrqu->name, "IEEE 802.11bg");

	return 0;
}

/*
 * ps3_jupiter_ap_get_nick
 */
static int ps3_jupiter_ap_get_nick(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	strcpy(extra, "ps3_jupiter_ap");
	wrqu->data.length = strlen(extra);
	wrqu->data.flags = 1;

	return 0;
}

/*
 * ps3_jupiter_ap_get_range
 */
static int ps3_jupiter_ap_get_range(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);
	struct iw_point *point = &wrqu->data;
	struct iw_range *range = (struct iw_range *) extra;
	unsigned int i, chan;

	point->length = sizeof(struct iw_range);
	memset(range, 0, sizeof(struct iw_range));

	range->we_version_compiled = WIRELESS_EXT;
	range->we_version_source = 22;

	for (i = 0, chan = 0;
	     (i < ARRAY_SIZE(ps3_jupiter_ap_channel_freq)) && (chan < IW_MAX_FREQUENCIES); i++) {
		if (japd->channel_info & (1 << i)) {
			range->freq[chan].i = i + 1;
			range->freq[chan].m = ps3_jupiter_ap_channel_freq[i];
			range->freq[chan].e = 6;
			chan++;
		}
	}

	range->num_frequency = chan;
	range->old_num_frequency = chan;
	range->num_channels = chan;
	range->old_num_channels = chan;

	for (i = 0; i < ARRAY_SIZE(ps3_jupiter_ap_bitrate); i++)
		range->bitrate[i] = ps3_jupiter_ap_bitrate[i];
	range->num_bitrates = i;

	range->max_qual.qual = 100;
	range->max_qual.level = 100;
	range->avg_qual.qual = 50;
	range->avg_qual.level = 50;
	range->sensitivity = 0;

	IW_EVENT_CAPA_SET_KERNEL(range->event_capa);

	range->enc_capa = IW_ENC_CAPA_WPA | IW_ENC_CAPA_WPA2 |
	    IW_ENC_CAPA_CIPHER_TKIP | IW_ENC_CAPA_CIPHER_CCMP |
	    IW_ENC_CAPA_4WAY_HANDSHAKE;

	range->encoding_size[0] = 5;
	range->encoding_size[1] = 13;
	range->encoding_size[2] = 32;
	range->num_encoding_sizes = 3;
	range->max_encoding_tokens = 4;

	range->scan_capa = IW_SCAN_CAPA_ESSID | IW_SCAN_CAPA_CHANNEL;

	return 0;
}

/*
 * ps3_jupiter_ap_set_mode
 */
static int ps3_jupiter_ap_set_mode(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	switch (wrqu->mode) {
	case IW_MODE_MASTER:
	break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

/*
 * ps3_jupiter_ap_get_mode
 */
static int ps3_jupiter_ap_get_mode(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	wrqu->mode = IW_MODE_MASTER;

	return 0;
}

/*
 * ps3_jupiter_ap_set_freq
 */
static int ps3_jupiter_ap_set_freq(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);
	struct iw_freq *freq = &wrqu->freq;
	__u8 channel;
	unsigned long irq_flags;
	int err;

	spin_lock_irqsave(&japd->lock, irq_flags);

	if (!freq->m) {
		japd->channel = 0;
		clear_bit(PS3_JUPITER_AP_CONFIG_CHANNEL_SET, &japd->config_status);
	} else {
		if (freq->e == 1)
			channel = ps3_jupiter_ap_freq_to_channel(freq->m / 100000);
		else
			channel = freq->m;

		if (!channel || !(japd->channel_info & (1 << (channel - 1)))) {
			err = -EINVAL;
			goto done;
		}

		pr_debug("%s: channel %d\n", __func__, channel);

		japd->channel = channel;
		set_bit(PS3_JUPITER_AP_CONFIG_CHANNEL_SET, &japd->config_status);
	}

	err = 0;

done:

	spin_unlock_irqrestore(&japd->lock, irq_flags);

	return err;
}

/*
 * ps3_jupiter_ap_get_freq
 */
static int ps3_jupiter_ap_get_freq(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);
	struct iw_freq *freq = &wrqu->freq;
	unsigned long irq_flags;

	pr_debug("%s: called\n", __func__);

	spin_lock_irqsave(&japd->lock, irq_flags);

	if (test_bit(PS3_JUPITER_AP_CONFIG_CHANNEL_SET, &japd->config_status)) {
		freq->e = 1;
		freq->m = ps3_jupiter_ap_channel_freq[japd->channel - 1] * 100000;
	} else {
		freq->e = 0;
		freq->m = 0;
	}

	pr_debug("%s: done\n", __func__);

	spin_unlock_irqrestore(&japd->lock, irq_flags);

	return 0;
}

/*
 * ps3_jupiter_ap_set_auth
 */
static int ps3_jupiter_ap_set_auth(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);
	struct iw_param *param = &wrqu->param;
	unsigned long irq_flags;
	int err = 0;

	spin_lock_irqsave(&japd->lock, irq_flags);

	switch (param->flags & IW_AUTH_INDEX) {
	case IW_AUTH_WPA_VERSION:
		if (param->value & IW_AUTH_WPA_VERSION_DISABLED) {
			japd->wpa_mode = PS3_JUPITER_AP_WPA_MODE_NONE;
			japd->group_cipher_mode = PS3_JUPITER_AP_CIPHER_WEP;
			japd->pairwise_cipher_mode = PS3_JUPITER_AP_CIPHER_WEP;
		} else if (param->value & IW_AUTH_WPA_VERSION_WPA) {
			japd->wpa_mode = PS3_JUPITER_AP_WPA_MODE_WPA;
			japd->group_cipher_mode = PS3_JUPITER_AP_CIPHER_TKIP;
			japd->pairwise_cipher_mode = PS3_JUPITER_AP_CIPHER_TKIP;
		} else if (param->value & IW_AUTH_WPA_VERSION_WPA2) {
			japd->wpa_mode = PS3_JUPITER_AP_WPA_MODE_WPA2;
			japd->group_cipher_mode = PS3_JUPITER_AP_CIPHER_AES;
			japd->pairwise_cipher_mode = PS3_JUPITER_AP_CIPHER_AES;
		}
	break;
	case IW_AUTH_CIPHER_GROUP:
		if (param->value & IW_AUTH_CIPHER_NONE)
			japd->group_cipher_mode = PS3_JUPITER_AP_CIPHER_NONE;
		else if (param->value & (IW_AUTH_CIPHER_WEP40 | IW_AUTH_CIPHER_WEP104))
			japd->group_cipher_mode = PS3_JUPITER_AP_CIPHER_WEP;
		else if (param->value & IW_AUTH_CIPHER_TKIP)
			japd->group_cipher_mode = PS3_JUPITER_AP_CIPHER_TKIP;
		else if (param->value & IW_AUTH_CIPHER_CCMP)
			japd->group_cipher_mode = PS3_JUPITER_AP_CIPHER_AES;
	break;
	case IW_AUTH_CIPHER_PAIRWISE:
		if (param->value & IW_AUTH_CIPHER_NONE)
			japd->pairwise_cipher_mode = PS3_JUPITER_AP_CIPHER_NONE;
		else if (param->value & (IW_AUTH_CIPHER_WEP40 | IW_AUTH_CIPHER_WEP104))
			japd->pairwise_cipher_mode = PS3_JUPITER_AP_CIPHER_WEP;
		else if (param->value & IW_AUTH_CIPHER_TKIP)
			japd->pairwise_cipher_mode = PS3_JUPITER_AP_CIPHER_TKIP;
		else if (param->value & IW_AUTH_CIPHER_CCMP)
			japd->pairwise_cipher_mode = PS3_JUPITER_AP_CIPHER_AES;
	break;
	case IW_AUTH_80211_AUTH_ALG:
		if (param->value & IW_AUTH_ALG_OPEN_SYSTEM)
			japd->auth_mode = PS3_JUPITER_AP_AUTH_OPEN;
		else if (param->value & IW_AUTH_ALG_SHARED_KEY)
			japd->auth_mode = PS3_JUPITER_AP_AUTH_SHARED_KEY;
		else
			err = -EINVAL;
	break;
	case IW_AUTH_WPA_ENABLED:
		if (param->value)
			japd->wpa_mode = PS3_JUPITER_AP_WPA_MODE_WPA;
		else
			japd->wpa_mode = PS3_JUPITER_AP_WPA_MODE_NONE;
	break;
	case IW_AUTH_KEY_MGMT:
		if (!(param->value & IW_AUTH_KEY_MGMT_PSK))
			err = -EOPNOTSUPP;
	break;
	default:
		err = -EOPNOTSUPP;
	}

	spin_unlock_irqrestore(&japd->lock, irq_flags);

	return err;
}

/*
 * ps3_jupiter_ap_get_auth
 */
static int ps3_jupiter_ap_get_auth(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);
	struct iw_param *param = &wrqu->param;
	unsigned long irq_flags;
	int err = 0;

	spin_lock_irqsave(&japd->lock, irq_flags);

	switch (param->flags & IW_AUTH_INDEX) {
	case IW_AUTH_WPA_VERSION:
	switch (japd->wpa_mode) {
	case PS3_JUPITER_AP_WPA_MODE_WPA:
		param->value |= IW_AUTH_WPA_VERSION_WPA;
	break;
	case PS3_JUPITER_AP_WPA_MODE_WPA2:
		param->value |= IW_AUTH_WPA_VERSION_WPA2;
	break;
	default:
		param->value |= IW_AUTH_WPA_VERSION_DISABLED;
	}
	break;
	case IW_AUTH_80211_AUTH_ALG:
	switch (japd->auth_mode) {
	case PS3_JUPITER_AP_AUTH_OPEN:
		param->value |= IW_AUTH_ALG_OPEN_SYSTEM;
	break;
	case PS3_JUPITER_AP_AUTH_SHARED_KEY:
		param->value |= IW_AUTH_ALG_SHARED_KEY;
	break;
	}
	break;
	case IW_AUTH_WPA_ENABLED:
	switch (japd->wpa_mode) {
	case PS3_JUPITER_AP_WPA_MODE_WPA:
	case PS3_JUPITER_AP_WPA_MODE_WPA2:
		param->value = 1;
	break;
	default:
		param->value = 0;
	}
	break;
	default:
		err = -EOPNOTSUPP;
	}

	spin_unlock_irqrestore(&japd->lock, irq_flags);

	return err;
}

/*
 * ps3_jupiter_ap_set_essid
 */
static int ps3_jupiter_ap_set_essid(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);
	unsigned long irq_flags;

	pr_debug("%s: called\n", __func__);

	if (wrqu->essid.length > IW_ESSID_MAX_SIZE)
		return -EINVAL;

	spin_lock_irqsave(&japd->lock, irq_flags);

	if (wrqu->essid.flags) {
		memcpy(japd->essid, extra, wrqu->essid.length);
		japd->essid_length = wrqu->essid.length;
		set_bit(PS3_JUPITER_AP_CONFIG_ESSID_SET, &japd->config_status);

		pr_debug("%s: essid %s\n", __func__, extra);
	} else {
		clear_bit(PS3_JUPITER_AP_CONFIG_ESSID_SET, &japd->config_status);

		pr_debug("%s: essid any\n", __func__);
	}

	spin_unlock_irqrestore(&japd->lock, irq_flags);

	pr_debug("%s: done\n", __func__);

	return 0;
}

/*
 * ps3_jupiter_ap_get_essid
 */
static int ps3_jupiter_ap_get_essid(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);
	unsigned long irq_flags;

	spin_lock_irqsave(&japd->lock, irq_flags);

	if (test_bit(PS3_JUPITER_AP_CONFIG_ESSID_SET, &japd->config_status)) {
		memcpy(extra, japd->essid, japd->essid_length);
		wrqu->essid.length = japd->essid_length;
		wrqu->essid.flags = 1;
	} else {
		wrqu->essid.flags = 0;
	}

	spin_unlock_irqrestore(&japd->lock, irq_flags);

	return 0;
}

/*
 * ps3_jupiter_ap_set_encode
 */
static int ps3_jupiter_ap_set_encode(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);
	struct iw_point *enc = &wrqu->encoding;
	__u16 flags = enc->flags & IW_ENCODE_FLAGS;
	int key_index = enc->flags & IW_ENCODE_INDEX;
	int key_index_provided;
	unsigned long irq_flags;
	int err = 0;

	if (key_index > PS3_JUPITER_AP_WEP_KEYS)
		return -EINVAL;

	spin_lock_irqsave(&japd->lock, irq_flags);

	if (key_index) {
		key_index--;
		key_index_provided = 1;
	} else {
		key_index = japd->curr_key_index;
		key_index_provided = 0;
	}

	if (flags & IW_ENCODE_NOKEY) {
		if (!(flags & ~IW_ENCODE_NOKEY) && key_index_provided) {
			japd->curr_key_index = key_index;
			goto done;
		}

		if (flags & IW_ENCODE_DISABLED) {
			if (key_index_provided) {
				clear_bit(key_index, &japd->key_config_status);
			} else {
				japd->group_cipher_mode = PS3_JUPITER_AP_CIPHER_NONE;
				japd->pairwise_cipher_mode = PS3_JUPITER_AP_CIPHER_NONE;
				japd->key_config_status = 0;
			}
		}

		if (flags & IW_ENCODE_OPEN)
			japd->auth_mode = PS3_JUPITER_AP_AUTH_OPEN;
		else if (flags & IW_ENCODE_RESTRICTED)
			japd->auth_mode = PS3_JUPITER_AP_AUTH_SHARED_KEY;
	} else {
		if (enc->length > IW_ENCODING_TOKEN_MAX) {
			err = -EINVAL;
			goto done;
		}

		memcpy(japd->key[key_index], extra, enc->length);
		japd->key_length[key_index] = enc->length;
		set_bit(key_index, &japd->key_config_status);

		japd->group_cipher_mode = PS3_JUPITER_AP_CIPHER_WEP;
		japd->pairwise_cipher_mode = PS3_JUPITER_AP_CIPHER_WEP;
	}

done:

	spin_unlock_irqrestore(&japd->lock, irq_flags);

	return err;
}

/*
 * ps3_jupiter_ap_get_encode
 */
static int ps3_jupiter_ap_get_encode(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);
	struct iw_point *enc = &wrqu->encoding;
	int key_index = enc->flags & IW_ENCODE_INDEX;
	int key_index_provided;
	unsigned long irq_flags;
	int err = 0;

	if (key_index > PS3_JUPITER_AP_WEP_KEYS)
		return -EINVAL;

	spin_lock_irqsave(&japd->lock, irq_flags);

	if (key_index) {
		key_index--;
		key_index_provided = 1;
	} else {
		key_index = japd->curr_key_index;
		key_index_provided = 0;
	}

	if (japd->group_cipher_mode == PS3_JUPITER_AP_CIPHER_WEP) {
		switch (japd->auth_mode) {
		case PS3_JUPITER_AP_AUTH_OPEN:
			enc->flags |= IW_ENCODE_OPEN;
			break;
		case PS3_JUPITER_AP_AUTH_SHARED_KEY:
			enc->flags |= IW_ENCODE_RESTRICTED;
			break;
		}
	} else {
		enc->flags = IW_ENCODE_DISABLED;
	}

	if (test_bit(key_index, &japd->key_config_status)) {
		if (enc->length < japd->key_length[key_index]) {
			err = -EINVAL;
			goto done;
		}

		memcpy(extra, japd->key[key_index], japd->key_length[key_index]);
		enc->length = japd->key_length[key_index];
	} else {
		enc->length = 0;
		enc->flags |= IW_ENCODE_NOKEY;
	}

	enc->flags |= (key_index + 1);

done:

	spin_unlock_irqrestore(&japd->lock, irq_flags);

	return err;
}

/*
 * ps3_jupiter_ap_set_encodeext
 */
static int ps3_jupiter_ap_set_encodeext(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);
	struct iw_point *enc = &wrqu->encoding;
	struct iw_encode_ext *enc_ext = (struct iw_encode_ext *) extra;
	__u16 flags = enc->flags & IW_ENCODE_FLAGS;
	int key_index = enc->flags & IW_ENCODE_INDEX;
	unsigned long irq_flags;
	int err = 0;

	if (key_index > PS3_JUPITER_AP_WEP_KEYS)
		return -EINVAL;

	spin_lock_irqsave(&japd->lock, irq_flags);

	if (key_index)
		key_index--;
	else
		key_index = japd->curr_key_index;

	if (!enc->length && (enc_ext->ext_flags & IW_ENCODE_EXT_SET_TX_KEY)) {
		japd->curr_key_index = key_index;
	} else if ((enc_ext->alg == IW_ENCODE_ALG_NONE) || (flags & IW_ENCODE_DISABLED)) {
		japd->auth_mode = PS3_JUPITER_AP_AUTH_OPEN;
		japd->wpa_mode = PS3_JUPITER_AP_WPA_MODE_NONE;
		japd->group_cipher_mode = PS3_JUPITER_AP_CIPHER_NONE;
		japd->pairwise_cipher_mode = PS3_JUPITER_AP_CIPHER_NONE;
	} else if (enc_ext->alg == IW_ENCODE_ALG_WEP) {
		if (flags & IW_ENCODE_OPEN)
			japd->auth_mode = PS3_JUPITER_AP_AUTH_OPEN;
		else if (flags & IW_ENCODE_RESTRICTED)
			japd->auth_mode = PS3_JUPITER_AP_AUTH_SHARED_KEY;

		if (enc_ext->key_len > IW_ENCODING_TOKEN_MAX) {
			err = -EINVAL;
			goto done;
		}

		memcpy(japd->key[key_index], enc_ext->key, enc_ext->key_len);
		japd->key_length[key_index] = enc_ext->key_len;
		set_bit(key_index, &japd->key_config_status);
	} else if (enc_ext->alg == IW_ENCODE_ALG_PMK) {
		if (enc_ext->key_len != PS3_JUPITER_AP_WPA_PSK_LENGTH) {
			err = -EINVAL;
			goto done;
		}

		memcpy(japd->psk, enc_ext->key, enc_ext->key_len);
		japd->psk_length = enc_ext->key_len;
		japd->psk_passphrase = 0;
		set_bit(PS3_JUPITER_AP_CONFIG_WPA_PSK_SET, &japd->config_status);
	}

done:

	spin_unlock_irqrestore(&japd->lock, irq_flags);

	return err;
}

/*
 * ps3_jupiter_ap_get_encodeext
 */
static int ps3_jupiter_ap_get_encodeext(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);
	struct iw_point *enc = &wrqu->encoding;
	struct iw_encode_ext *enc_ext = (struct iw_encode_ext *) extra;
	int key_index = enc->flags & IW_ENCODE_INDEX;
	unsigned long irq_flags;
	int err = 0;

	if ((enc->length - sizeof(struct iw_encode_ext)) < 0)
		return -EINVAL;

	if (key_index > PS3_JUPITER_AP_WEP_KEYS)
		return -EINVAL;

	spin_lock_irqsave(&japd->lock, irq_flags);

	if (key_index)
		key_index--;
	else
		key_index = japd->curr_key_index;

	memset(enc_ext, 0, sizeof(*enc_ext));

	switch (japd->group_cipher_mode) {
	case PS3_JUPITER_AP_CIPHER_WEP:
		enc_ext->alg = IW_ENCODE_ALG_WEP;
		enc->flags |= IW_ENCODE_ENABLED;
	break;
	case PS3_JUPITER_AP_CIPHER_TKIP:
		enc_ext->alg = IW_ENCODE_ALG_TKIP;
		enc->flags |= IW_ENCODE_ENABLED;
	break;
	case PS3_JUPITER_AP_CIPHER_AES:
		enc_ext->alg = IW_ENCODE_ALG_CCMP;
		enc->flags |= IW_ENCODE_ENABLED;
	break;
	default:
		enc_ext->alg = IW_ENCODE_ALG_NONE;
		enc->flags |= IW_ENCODE_NOKEY;
	}

	if (!(enc->flags & IW_ENCODE_NOKEY)) {
		if ((enc->length - sizeof(struct iw_encode_ext)) < japd->key_length[key_index]) {
			err = -E2BIG;
			goto done;
		}

		if (test_bit(key_index, &japd->key_config_status))
			memcpy(enc_ext->key, japd->key[key_index], japd->key_length[key_index]);
	}

done:

	spin_unlock_irqrestore(&japd->lock, irq_flags);

	return err;
}

/*
 * ps3_jupiter_ap_set_genie
 */
static int ps3_jupiter_ap_set_genie(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	/* XXX: implement */

	return 0;
}

/*
 * ps3_jupiter_ap_get_genie
 */
static int ps3_jupiter_ap_get_genie(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	/* XXX: implement */

	return 0;
}

/*
 * ps3_jupiter_ap_set_mlme
 */
static int ps3_jupiter_ap_set_mlme(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	/* XXX: implement */

	return 0;
}

#ifdef CONFIG_WEXT_PRIV
/*
 * ps3_jupiter_ap_set_opmode
 */
static int ps3_jupiter_ap_set_opmode(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);
	int opmode = *(int *) extra;
	unsigned long irq_flags;
	int err = 0;

	spin_lock_irqsave(&japd->lock, irq_flags);

	switch (opmode) {
	case PS3_JUPITER_AP_OPMODE_11B:
	case PS3_JUPITER_AP_OPMODE_11G:
	case PS3_JUPITER_AP_OPMODE_11BG:
		japd->opmode = opmode;
	break;
	default:
		err = -EINVAL;
	}

	spin_unlock_irqrestore(&japd->lock, irq_flags);

	return err;
}

/*
 * ps3_jupiter_ap_get_opmode
 */
static int ps3_jupiter_ap_get_opmode(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);
	unsigned long irq_flags;

	spin_lock_irqsave(&japd->lock, irq_flags);

	memcpy(extra, &japd->opmode, sizeof(japd->opmode));
	wrqu->data.length = sizeof(japd->opmode);

	spin_unlock_irqrestore(&japd->lock, irq_flags);

	return 0;
}

/*
 * ps3_jupiter_ap_set_wpa_mode
 */
static int ps3_jupiter_ap_set_wpa_mode(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);
	int wpa_mode = *(int *) extra;
	unsigned long irq_flags;
	int err = 0;

	spin_lock_irqsave(&japd->lock, irq_flags);

	switch (wpa_mode) {
	case PS3_JUPITER_AP_WPA_MODE_NONE:
	case PS3_JUPITER_AP_WPA_MODE_WPA:
	case PS3_JUPITER_AP_WPA_MODE_WPA2:
		japd->wpa_mode = wpa_mode;
	break;
	default:
		err = -EINVAL;
	}

	spin_unlock_irqrestore(&japd->lock, irq_flags);

	return err;
}

/*
 * ps3_jupiter_ap_get_wpa_mode
 */
static int ps3_jupiter_ap_get_wpa_mode(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);
	unsigned long irq_flags;

	spin_lock_irqsave(&japd->lock, irq_flags);

	memcpy(extra, &japd->wpa_mode, sizeof(japd->wpa_mode));
	wrqu->data.length = sizeof(japd->wpa_mode);

	spin_unlock_irqrestore(&japd->lock, irq_flags);

	return 0;
}

/*
 * ps3_jupiter_ap_set_wpa_group
 */
static int ps3_jupiter_ap_set_wpa_group(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);
	int group_cipher_mode = *(int *) extra;
	unsigned long irq_flags;
	int err = 0;

	spin_lock_irqsave(&japd->lock, irq_flags);

	switch (group_cipher_mode) {
	case PS3_JUPITER_AP_CIPHER_TKIP:
	case PS3_JUPITER_AP_CIPHER_AES:
		japd->group_cipher_mode = group_cipher_mode;
	break;
	default:
		err = -EINVAL;
	}

	spin_unlock_irqrestore(&japd->lock, irq_flags);

	return err;
}

/*
 * ps3_jupiter_ap_get_wpa_group
 */
static int ps3_jupiter_ap_get_wpa_group(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);
	unsigned long irq_flags;

	spin_lock_irqsave(&japd->lock, irq_flags);

	memcpy(extra, &japd->group_cipher_mode, sizeof(japd->group_cipher_mode));
	wrqu->data.length = sizeof(japd->group_cipher_mode);

	spin_unlock_irqrestore(&japd->lock, irq_flags);

	return 0;
}

/*
 * ps3_jupiter_ap_set_wpa_pairwise
 */
static int ps3_jupiter_ap_set_wpa_pairwise(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);
	int pairwise_cipher_mode = *(int *) extra;
	unsigned long irq_flags;
	int err = 0;

	spin_lock_irqsave(&japd->lock, irq_flags);

	switch (pairwise_cipher_mode) {
	case PS3_JUPITER_AP_CIPHER_TKIP:
	case PS3_JUPITER_AP_CIPHER_AES:
		japd->pairwise_cipher_mode = pairwise_cipher_mode;
	break;
	default:
		err = -EINVAL;
	}

	spin_unlock_irqrestore(&japd->lock, irq_flags);

	return err;
}

/*
 * ps3_jupiter_ap_get_wpa_pairwise
 */
static int ps3_jupiter_ap_get_wpa_pairwise(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);
	unsigned long irq_flags;

	spin_lock_irqsave(&japd->lock, irq_flags);

	memcpy(extra, &japd->pairwise_cipher_mode, sizeof(japd->pairwise_cipher_mode));
	wrqu->data.length = sizeof(japd->pairwise_cipher_mode);

	spin_unlock_irqrestore(&japd->lock, irq_flags);

	return 0;
}

/*
 * ps3_jupiter_ap_set_wpa_psk
 */
static int ps3_jupiter_ap_set_wpa_psk(struct net_device *netdev,
	struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);
	unsigned long irq_flags;
	int i;
	int err = 0;

	pr_debug("%s: psk '%s'\n", __func__, extra);

	if ((wrqu->data.length - 1) < 8)
		return -EINVAL;

	spin_lock_irqsave(&japd->lock, irq_flags);

	if ((wrqu->data.length - 1) != (PS3_JUPITER_AP_WPA_PSK_LENGTH * 2)) {
		memcpy(japd->psk, extra, wrqu->data.length - 1);
		japd->psk_length = wrqu->data.length - 1;
		japd->psk_passphrase = 1;
	} else {
		for (i = 0; i < wrqu->data.length - 1; i += 2) {
			if (!isxdigit(extra[i]) || !isxdigit(extra[i + 1])) {
				err = -EINVAL;
				goto done;
			}

			if (isdigit(extra[i + 1]))
				japd->psk[i / 2] = extra[i + 1] - '0';
			else
				japd->psk[i / 2] = tolower(extra[i + 1]) - 'a';

			if (isdigit(extra[i]))
				japd->psk[i / 2] += (extra[i] - '0') * 16;
			else
				japd->psk[i / 2] += (tolower(extra[i]) - 'a') * 16;
		}

		japd->psk_length = PS3_JUPITER_AP_WPA_PSK_LENGTH;
		japd->psk_passphrase = 0;
	}

	set_bit(PS3_JUPITER_AP_CONFIG_WPA_PSK_SET, &japd->config_status);

done:

	spin_unlock_irqrestore(&japd->lock, irq_flags);

	return err;
}
#endif

/*
 * ps3_jupiter_ap_get_wireless_stats
 */
static struct iw_statistics *ps3_jupiter_ap_get_wireless_stats(struct net_device *netdev)
{
	/* XXX: implement */

	return NULL;
}

/*
 * ps3_jupiter_ap_open
 */
static int ps3_jupiter_ap_open(struct net_device *netdev)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);

	pr_debug("%s: called\n", __func__);

	usb_unpoison_anchored_urbs(&japd->tx_urb_anchor);

	ps3_jupiter_ap_start_ap(japd);

	netif_start_queue(netdev);

	pr_debug("%s: done\n", __func__);

	return 0;
}

/*
 * ps3_jupiter_ap_stop
 */
static int ps3_jupiter_ap_stop(struct net_device *netdev)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);

	pr_debug("%s: called\n", __func__);

	netif_stop_queue(netdev);

	cancel_delayed_work(&japd->start_ap_work);

	if (japd->start_ap_status != PS3_JUPITER_AP_STOPPED)
		ps3_jupiter_ap_stop_ap(japd);

	tasklet_kill(&japd->rx_tasklet);
	ps3_jupiter_ap_purge_rx_skb_queue(japd);

	ps3_jupiter_ap_free_tx_urbs(japd);

	ps3_jupiter_ap_reset_state(japd);

	pr_debug("%s: done\n", __func__);

	return 0;
}

/*
 * ps3_jupiter_ap_start_xmit
 */
static int ps3_jupiter_ap_start_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	struct ps3_jupiter_ap_dev *japd = netdev_priv(netdev);

	return ps3_jupiter_ap_tx_skb(japd, skb);
}

/*
 * ps3_jupiter_ap_set_rx_mode
 */
static void ps3_jupiter_ap_set_rx_mode(struct net_device *netdev)
{
	/* XXX: implement */
}

/*
 * ps3_jupiter_ap_change_mtu
 */
static int ps3_jupiter_ap_change_mtu(struct net_device *netdev, int new_mtu)
{
	/* XXX: implement */

	return 0;
}

/*
 * ps3_jupiter_ap_tx_timeout
 */
static void ps3_jupiter_ap_tx_timeout(struct net_device *netdev)
{
	/* XXX: implement */
}

/*
 * ps3_jupiter_ap_get_drvinfo
 */
static void ps3_jupiter_ap_get_drvinfo(struct net_device *netdev, struct ethtool_drvinfo *info)
{
	/* XXX: implement */
}

/*
 * ps3_jupiter_ap_get_link
 */
static u32 ps3_jupiter_ap_get_link(struct net_device *netdev)
{
	/* XXX: implement */

	return 0;
}

static const iw_handler ps3_jupiter_ap_iw_handler[] =
{
	IW_HANDLER(SIOCGIWNAME,		ps3_jupiter_ap_get_name),
	IW_HANDLER(SIOCGIWNICKN,	ps3_jupiter_ap_get_nick),
	IW_HANDLER(SIOCGIWRANGE,	ps3_jupiter_ap_get_range),
	IW_HANDLER(SIOCSIWMODE,		ps3_jupiter_ap_set_mode),
	IW_HANDLER(SIOCGIWMODE,		ps3_jupiter_ap_get_mode),
	IW_HANDLER(SIOCSIWFREQ,		ps3_jupiter_ap_set_freq),
	IW_HANDLER(SIOCGIWFREQ,		ps3_jupiter_ap_get_freq),
	IW_HANDLER(SIOCSIWAUTH,		ps3_jupiter_ap_set_auth),
	IW_HANDLER(SIOCGIWAUTH,		ps3_jupiter_ap_get_auth),
	IW_HANDLER(SIOCSIWESSID,	ps3_jupiter_ap_set_essid),
	IW_HANDLER(SIOCGIWESSID,	ps3_jupiter_ap_get_essid),
	IW_HANDLER(SIOCSIWENCODE,	ps3_jupiter_ap_set_encode),
	IW_HANDLER(SIOCGIWENCODE,	ps3_jupiter_ap_get_encode),
	IW_HANDLER(SIOCSIWENCODEEXT,	ps3_jupiter_ap_set_encodeext),
	IW_HANDLER(SIOCGIWENCODEEXT,	ps3_jupiter_ap_get_encodeext),
	IW_HANDLER(SIOCSIWGENIE,	ps3_jupiter_ap_set_genie),
	IW_HANDLER(SIOCGIWGENIE,	ps3_jupiter_ap_get_genie),
	IW_HANDLER(SIOCSIWMLME,		ps3_jupiter_ap_set_mlme),
};

#ifdef CONFIG_WEXT_PRIV
static const iw_handler ps3_jupiter_ap_iw_priv_handler[] = {
	ps3_jupiter_ap_set_opmode,
	ps3_jupiter_ap_get_opmode,
	ps3_jupiter_ap_set_wpa_mode,
	ps3_jupiter_ap_get_wpa_mode,
	ps3_jupiter_ap_set_wpa_group,
	ps3_jupiter_ap_get_wpa_group,
	ps3_jupiter_ap_set_wpa_pairwise,
	ps3_jupiter_ap_get_wpa_pairwise,
	ps3_jupiter_ap_set_wpa_psk,
};

enum {
	PS3_JUPITER_AP_IW_PRIV_SET_OPMODE = SIOCIWFIRSTPRIV,
	PS3_JUPITER_AP_IW_PRIV_GET_OPMODE,
	PS3_JUPITER_AP_IW_PRIV_SET_WPA_MODE,
	PS3_JUPITER_AP_IW_PRIV_GET_WPA_MODE,
	PS3_JUPITER_AP_IW_PRIV_SET_WPA_GROUP,
	PS3_JUPITER_AP_IW_PRIV_GET_WPA_GROUP,
	PS3_JUPITER_AP_IW_PRIV_SET_WPA_PAIRWISE,
	PS3_JUPITER_AP_IW_PRIV_GET_WPA_PAIRWISE,
	PS3_JUPITER_AP_IW_PRIV_SET_WPA_PSK,
};

static struct iw_priv_args ps3_jupiter_ap_iw_priv_args[] = {
	{
		.cmd = PS3_JUPITER_AP_IW_PRIV_SET_OPMODE,
		.set_args = IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
		.name = "set_opmode"
	},
	{
		.cmd = PS3_JUPITER_AP_IW_PRIV_GET_OPMODE,
		.get_args = IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
		.name = "get_opmode"
	},
	{
		.cmd = PS3_JUPITER_AP_IW_PRIV_SET_WPA_MODE,
		.set_args = IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
		.name = "set_wpa_mode"
	},
	{
		.cmd = PS3_JUPITER_AP_IW_PRIV_GET_WPA_MODE,
		.get_args = IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
		.name = "get_wpa_mode"
	},
	{
		.cmd = PS3_JUPITER_AP_IW_PRIV_SET_WPA_GROUP,
		.set_args = IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
		.name = "set_wpa_group"
	},
	{
		.cmd = PS3_JUPITER_AP_IW_PRIV_GET_WPA_GROUP,
		.get_args = IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
		.name = "get_wpa_group"
	},
	{
		.cmd = PS3_JUPITER_AP_IW_PRIV_SET_WPA_PAIRWISE,
		.set_args = IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
		.name = "set_wpa_pairwise"
	},
	{
		.cmd = PS3_JUPITER_AP_IW_PRIV_GET_WPA_PAIRWISE,
		.get_args = IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
		.name = "get_wpa_pairwise"
	},
	{
		.cmd = PS3_JUPITER_AP_IW_PRIV_SET_WPA_PSK,
		.set_args = IW_PRIV_TYPE_CHAR | (PS3_JUPITER_AP_WPA_PSK_LENGTH * 2 + 1),
		.name = "set_wpa_psk"
	},
};
#endif

static const struct iw_handler_def ps3_jupiter_ap_iw_handler_def = {
	.standard		= ps3_jupiter_ap_iw_handler,
	.num_standard		= ARRAY_SIZE(ps3_jupiter_ap_iw_handler),
#ifdef CONFIG_WEXT_PRIV
	.private		= ps3_jupiter_ap_iw_priv_handler,
	.num_private		= ARRAY_SIZE(ps3_jupiter_ap_iw_priv_handler),
	.private_args		= ps3_jupiter_ap_iw_priv_args,
	.num_private_args	= ARRAY_SIZE(ps3_jupiter_ap_iw_priv_args),
#endif
	.get_wireless_stats	= ps3_jupiter_ap_get_wireless_stats,
};

static const struct net_device_ops ps3_jupiter_ap_net_device_ops = {
	.ndo_open		= ps3_jupiter_ap_open,
	.ndo_stop		= ps3_jupiter_ap_stop,
	.ndo_start_xmit		= ps3_jupiter_ap_start_xmit,
	.ndo_set_rx_mode	= ps3_jupiter_ap_set_rx_mode,
	.ndo_change_mtu		= ps3_jupiter_ap_change_mtu,
	.ndo_tx_timeout		= ps3_jupiter_ap_tx_timeout,
	.ndo_set_mac_address	= eth_mac_addr,
	.ndo_validate_addr	= eth_validate_addr,
};

static const struct ethtool_ops ps3_jupiter_ap_ethtool_ops = {
	.get_drvinfo	= ps3_jupiter_ap_get_drvinfo,
	.get_link	= ps3_jupiter_ap_get_link,
};

/*
 * ps3_jupiter_ap_rx_urb_complete
 */
static void ps3_jupiter_ap_rx_urb_complete(struct urb *urb)
{
	struct ps3_jupiter_ap_dev *japd = usb_get_intfdata(usb_ifnum_to_if(urb->dev, PS3_JUPITER_AP_IFACE));
	struct usb_device *udev = japd->udev;
	struct net_device *netdev = japd->netdev;
	struct sk_buff *skb = urb->context;
	int err;

	dev_dbg(&udev->dev, "Rx URB completed (%d)\n", urb->status);

	switch (urb->status) {
	case 0:
		/* success */

		if (urb->actual_length == 0x10) {
			dev_info(&udev->dev, "got empty Rx URB\n");
			break;
		}

		skb_put(skb, urb->actual_length);

		err = ps3_jupiter_ap_prepare_rx_urb(japd, urb);
		if (err) {
			dev_err(&udev->dev, "could not prepare Rx URB (%d)\n", err);
			break;
		}

		skb_queue_tail(&japd->rx_skb_queue, skb);

		tasklet_schedule(&japd->rx_tasklet);
	break;
	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
	case -ENODEV:
		goto free_skb;
	break;
	default:
		netdev->stats.rx_errors++;
		dev_err(&udev->dev, "Rx URB failed (%d)\n", urb->status);
	}

	/* resubmit */

	skb = urb->context;
	skb_reset_tail_pointer(skb);
	skb_trim(skb, 0);

	usb_anchor_urb(urb, &japd->rx_urb_anchor);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err) {
		netdev->stats.rx_errors++;
		usb_unanchor_urb(urb);
		dev_err(&udev->dev, "could not submit Rx URB (%d)\n", err);
		goto free_skb;
	}

	return;

free_skb:

	dev_kfree_skb_irq(skb);
}

/*
 * ps3_jupiter_ap_prepare_rx_urb
 */
static int ps3_jupiter_ap_prepare_rx_urb(struct ps3_jupiter_ap_dev *japd,
	struct urb *urb)
{
	struct usb_device *udev = japd->udev;
	struct sk_buff *skb;

	skb = dev_alloc_skb(PS3_JUPITER_AP_RX_BUFSIZE);
	if (!skb)
		return -ENOMEM;

	usb_fill_bulk_urb(urb, udev, usb_rcvbulkpipe(udev, PS3_JUPITER_AP_EP),
	    skb->data, PS3_JUPITER_AP_RX_BUFSIZE, ps3_jupiter_ap_rx_urb_complete, skb);

	return 0;
}

/*
 * ps3_jupiter_ap_alloc_rx_urbs
 */
static int ps3_jupiter_ap_alloc_rx_urbs(struct ps3_jupiter_ap_dev *japd)
{
	struct usb_device *udev = japd->udev;
	struct urb *urb;
	unsigned int i;
	int err;

	pr_debug("%s: called\n", __func__);

	init_usb_anchor(&japd->rx_urb_anchor);

	for (i = 0; i < PS3_JUPITER_AP_RX_URBS; i++) {
		urb = usb_alloc_urb(0, GFP_KERNEL);
		if (!urb) {
			dev_err(&udev->dev, "could not allocate Rx URB\n");
			err = -ENOMEM;
			goto done;
		}

		err = ps3_jupiter_ap_prepare_rx_urb(japd, urb);
		if (err) {
			dev_err(&udev->dev, "could not prepare Rx URB (%d)\n", err);
			usb_free_urb(urb);
			goto done;
		}

		usb_anchor_urb(urb, &japd->rx_urb_anchor);
		usb_free_urb(urb);

		err = usb_submit_urb(urb, GFP_KERNEL);
		if (err) {
			dev_err(&udev->dev, "could not submit Rx URB (%d)\n", err);
			dev_kfree_skb_any(urb->context);
			usb_unanchor_urb(urb);
			goto done;
		}
	}

	err = 0;

	pr_debug("%s: done\n", __func__);

done:

	if (err)
		usb_kill_anchored_urbs(&japd->rx_urb_anchor);

	return err;
}

/*
 * ps3_jupiter_ap_free_rx_urbs
 */
static void ps3_jupiter_ap_free_rx_urbs(struct ps3_jupiter_ap_dev *japd)
{
	usb_kill_anchored_urbs(&japd->rx_urb_anchor);

	usb_poison_anchored_urbs(&japd->rx_urb_anchor);
}

/*
 * ps3_jupiter_ap_purge_rx_skb_queue
 */
static void ps3_jupiter_ap_purge_rx_skb_queue(struct ps3_jupiter_ap_dev *japd)
{
	struct sk_buff *skb;
	unsigned long irq_flags;

	spin_lock_irqsave(&japd->rx_skb_queue.lock, irq_flags);

	while ((skb = __skb_dequeue(&japd->rx_skb_queue)))
		dev_kfree_skb_any(skb);

	spin_unlock_irqrestore(&japd->rx_skb_queue.lock, irq_flags);
}

/*
 * ps3_jupiter_ap_rx_tasklet
 */
static void ps3_jupiter_ap_rx_tasklet(unsigned long data)
{
	struct ps3_jupiter_ap_dev *japd = (struct ps3_jupiter_ap_dev *) data;
	struct net_device *netdev = japd->netdev;
	struct sk_buff *skb;

	while ((skb = skb_dequeue(&japd->rx_skb_queue))) {
		skb->protocol = eth_type_trans(skb, netdev);

		netdev->stats.rx_packets++;
		netdev->stats.rx_bytes += skb->len;

		netif_receive_skb(skb);
	}
}

/*
 * ps3_jupiter_ap_tx_urb_complete
 */
static void ps3_jupiter_ap_tx_urb_complete(struct urb *urb)
{
	struct ps3_jupiter_ap_dev *japd = usb_get_intfdata(usb_ifnum_to_if(urb->dev, PS3_JUPITER_AP_IFACE));
	struct usb_device *udev = japd->udev;
	struct net_device *netdev = japd->netdev;
	struct sk_buff *skb = urb->context;
	unsigned long irq_flags;

	dev_dbg(&udev->dev, "Tx URB completed (%d)\n", urb->status);

	switch (urb->status) {
	case 0:
		/* success */
	
		spin_lock_irqsave(&japd->lock, irq_flags);

		netdev->stats.tx_packets++;
		netdev->stats.tx_bytes += skb->len;

		spin_unlock_irqrestore(&japd->lock, irq_flags);

		atomic_dec(&japd->tx_submitted_urbs);
	break;
	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
	case -ENODEV:
	break;
	default:
		netdev->stats.tx_errors++;
		dev_err(&udev->dev, "Tx URB failed (%d)\n", urb->status);
	}

	dev_kfree_skb_irq(skb);
}

/*
 * ps3_jupiter_ap_tx_skb
 */
static int ps3_jupiter_ap_tx_skb(struct ps3_jupiter_ap_dev *japd, struct sk_buff *skb)
{
	struct usb_device *udev = japd->udev;
	struct net_device *netdev = japd->netdev;
	struct urb *urb;
	unsigned long irq_flags;
	int err;

	pr_debug("%s: called\n", __func__);

	if (japd->start_ap_status != PS3_JUPITER_AP_STARTED) {
		err = 0;
		goto drop;
	}

	spin_lock_irqsave(&japd->lock, irq_flags);

	if (atomic_read(&japd->tx_submitted_urbs) >= max_txurbs) {
		spin_unlock_irqrestore(&japd->lock, irq_flags);
		dev_err(&udev->dev, "no free Tx URBs\n");
		err = -EBUSY;
		goto drop;
	}

	atomic_inc(&japd->tx_submitted_urbs);

	spin_unlock_irqrestore(&japd->lock, irq_flags);

	urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (!urb) {
		atomic_dec(&japd->tx_submitted_urbs);
		dev_err(&udev->dev, "could not allocate Tx URB\n");
		err = -ENOMEM;
		goto drop;
	}

	usb_fill_bulk_urb(urb, udev, usb_sndbulkpipe(udev, PS3_JUPITER_AP_EP),
	    skb->data, skb->len, ps3_jupiter_ap_tx_urb_complete, skb);
	urb->transfer_flags |= URB_ZERO_PACKET;

	usb_anchor_urb(urb, &japd->tx_urb_anchor);
	usb_free_urb(urb);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err) {
		atomic_dec(&japd->tx_submitted_urbs);
		usb_unanchor_urb(urb);
		dev_err(&udev->dev, "could not submit Tx URB (%d)\n", err);
		goto drop;
	}

	pr_debug("%s: done\n", __func__);

	return 0;

drop:

	spin_lock_irqsave(&japd->lock, irq_flags);
	netdev->stats.tx_dropped++;
	spin_unlock_irqrestore(&japd->lock, irq_flags);
	dev_kfree_skb_any(skb);

	return err;
}

/*
 * ps3_jupiter_ap_free_tx_urbs
 */
static void ps3_jupiter_ap_free_tx_urbs(struct ps3_jupiter_ap_dev *japd)
{
	usb_wait_anchor_empty_timeout(&japd->tx_urb_anchor, msecs_to_jiffies(100));

	usb_kill_anchored_urbs(&japd->tx_urb_anchor);

	usb_poison_anchored_urbs(&japd->tx_urb_anchor);
}

/*
 * ps3_jupiter_ap_event_sta_connected
 */
static void ps3_jupiter_ap_event_sta_connected(struct ps3_jupiter_ap_dev *japd)
{
	struct usb_device *udev = japd->udev;

	dev_info(&udev->dev, "station connected\n");
}

/*
 * ps3_jupiter_ap_event_sta_disconnected
 */
static void ps3_jupiter_ap_event_sta_disconnected(struct ps3_jupiter_ap_dev *japd)
{
	struct usb_device *udev = japd->udev;

	dev_info(&udev->dev, "station disconnected\n");
}

/*
 * ps3_jupiter_ap_event_ap_started
 */
static void ps3_jupiter_ap_event_ap_started(struct ps3_jupiter_ap_dev *japd)
{
	struct usb_device *udev = japd->udev;

	dev_info(&udev->dev, "AP started\n");

	complete(&japd->start_ap_done_comp);
	netif_carrier_on(japd->netdev);
}

/*
 * ps3_jupiter_ap_event_ap_stopped
 */
static void ps3_jupiter_ap_event_ap_stopped(struct ps3_jupiter_ap_dev *japd)
{
	struct usb_device *udev = japd->udev;
	int start_ap_lock = 0;

	dev_info(&udev->dev, "AP stopped\n");

	if (mutex_trylock(&japd->start_ap_lock))
		start_ap_lock = 1;

	ps3_jupiter_ap_stop_ap(japd);

	japd->start_ap_status = PS3_JUPITER_AP_STOPPED;

	netif_carrier_off(japd->netdev);

	if (start_ap_lock)
		mutex_unlock(&japd->start_ap_lock);
}

/*
 * ps3_jupiter_ap_event_sta_wpa_connected
 */
static void ps3_jupiter_ap_event_sta_wpa_connected(struct ps3_jupiter_ap_dev *japd)
{
	struct usb_device *udev = japd->udev;

	dev_info(&udev->dev, "station WPA connected\n");
}

/*
 * ps3_jupiter_ap_event_handler
 */
static int ps3_jupiter_ap_event_handler(struct notifier_block *n,
	unsigned long val, void *v)
{
	struct ps3_jupiter_ap_dev *japd = container_of(n, struct ps3_jupiter_ap_dev, event_listener);
	struct usb_device *udev = japd->udev;
	struct ps3_eurus_event *event = v;

	dev_dbg(&udev->dev, "got event (0x%08x 0x%08x 0x%08x 0x%08x 0x%08x)\n",
	    event->hdr.type, event->hdr.id, event->hdr.timestamp,
	    event->hdr.payload_length, event->hdr.unknown);

	switch (event->hdr.type) {
	case PS3_EURUS_EVENT_TYPE_0x8:
		switch (event->hdr.id) {
		case PS3_EURUS_EVENT_STA_CONNECTED:
			ps3_jupiter_ap_event_sta_connected(japd);
		break;
		}
	break;
	case PS3_EURUS_EVENT_TYPE_0x10:
		switch (event->hdr.id) {
		case PS3_EURUS_EVENT_STA_DISCONNECTED:
			ps3_jupiter_ap_event_sta_disconnected(japd);
		break;
		case PS3_EURUS_EVENT_AP_STOPPED:
			ps3_jupiter_ap_event_ap_stopped(japd);
		break;
		}
	break;
	case PS3_EURUS_EVENT_TYPE_0x100:
		switch (event->hdr.id) {
		case PS3_EURUS_EVENT_AP_STARTED:
			ps3_jupiter_ap_event_ap_started(japd);
		break;
		case PS3_EURUS_EVENT_STA_WPA_CONNECTED:
			ps3_jupiter_ap_event_sta_wpa_connected(japd);
		break;
		}
	break;
	}

	return NOTIFY_OK;
}

/*
 * ps3_jupiter_ap_set_mac_addr
 */
static int ps3_jupiter_ap_set_mac_addr(struct ps3_jupiter_ap_dev *japd)
{
	struct usb_device *udev = japd->udev;
	struct net_device *netdev = japd->netdev;
	struct ps3_eurus_cmd_get_mac_addr_list *eurus_cmd_get_mac_addr_list;
	struct ps3_eurus_cmd_set_mac_addr *eurus_cmd_set_mac_addr;
	struct ps3_eurus_cmd_0x115b *eurus_cmd_0x115b;
	unsigned char *buf = NULL;
	unsigned int status, response_length;
	int err;

	buf = kmalloc(PS3_JUPITER_AP_CMD_BUFSIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	/* get MAC address list */

	eurus_cmd_get_mac_addr_list = (struct ps3_eurus_cmd_get_mac_addr_list *) buf;
	memset(eurus_cmd_get_mac_addr_list, 0, PS3_EURUS_MAC_ADDR_LIST_MAXSIZE);

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_GET_MAC_ADDR_LIST,
	    eurus_cmd_get_mac_addr_list, PS3_EURUS_MAC_ADDR_LIST_MAXSIZE, &status,
	    &response_length, eurus_cmd_get_mac_addr_list);
	if (err)
		goto done;

	if (status != PS3_EURUS_CMD_OK) {
		err = -EIO;
		goto done;
	}

	/* use second MAC address */

	memcpy(netdev->dev_addr, eurus_cmd_get_mac_addr_list->mac_addr + ETH_ALEN, ETH_ALEN);

	dev_info(&udev->dev, "MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
	    netdev->dev_addr[0], netdev->dev_addr[1], netdev->dev_addr[2],
	    netdev->dev_addr[3], netdev->dev_addr[4], netdev->dev_addr[5]);

	/* set MAC address */

	eurus_cmd_set_mac_addr = (struct ps3_eurus_cmd_set_mac_addr *) buf;
	memset(eurus_cmd_set_mac_addr, 0, sizeof(*eurus_cmd_set_mac_addr));
	memcpy(eurus_cmd_set_mac_addr->mac_addr, netdev->dev_addr, ETH_ALEN);

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_SET_MAC_ADDR,
	    eurus_cmd_set_mac_addr, sizeof(*eurus_cmd_set_mac_addr), &status, NULL, NULL);
	if (err)
		goto done;

	if (status != PS3_EURUS_CMD_OK) {
		err = -EIO;
		goto done;
	}

	eurus_cmd_0x115b = (struct ps3_eurus_cmd_0x115b *) buf;
	memset(eurus_cmd_0x115b, 0, sizeof(*eurus_cmd_0x115b));
	eurus_cmd_0x115b->unknown1 = cpu_to_le16(0x1);
	eurus_cmd_0x115b->unknown2 = cpu_to_le16(0x0);
	memcpy(eurus_cmd_0x115b->mac_addr, netdev->dev_addr, ETH_ALEN);

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x115b,
	    eurus_cmd_0x115b, sizeof(*eurus_cmd_0x115b), &status, NULL, NULL);
	if (err)
		goto done;

	if (status != PS3_EURUS_CMD_OK) {
		err = -EIO;
		goto done;
	}

	err = 0;

done:

	kfree(buf);

	return err;
}

/*
 * ps3_jupiter_ap_get_channel_info
 */
static int ps3_jupiter_ap_get_channel_info(struct ps3_jupiter_ap_dev *japd)
{
	struct ps3_eurus_cmd_get_channel_info *eurus_cmd_get_channel_info;
	unsigned char *buf = NULL;
	unsigned int status, response_length;
	int err;

	buf = kmalloc(PS3_JUPITER_AP_CMD_BUFSIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	eurus_cmd_get_channel_info = (struct ps3_eurus_cmd_get_channel_info *) buf;
	memset(eurus_cmd_get_channel_info, 0, sizeof(*eurus_cmd_get_channel_info));

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_GET_CHANNEL_INFO,
	    eurus_cmd_get_channel_info, sizeof(*eurus_cmd_get_channel_info), &status,
	    &response_length, eurus_cmd_get_channel_info);
	if (err)
		goto done;

	if (status != PS3_EURUS_CMD_OK) {
		err = -EIO;
		goto done;
	}

	japd->channel_info = eurus_cmd_get_channel_info->channel_info;

	err = 0;

done:

	kfree(buf);

	return err;
}

/*
 * ps3_jupiter_ap_config_and_start_ap
 */
static int ps3_jupiter_ap_config_and_start_ap(struct ps3_jupiter_ap_dev *japd)
{
	struct ps3_eurus_cmd_set_channel *eurus_cmd_set_channel;
	struct ps3_eurus_cmd_0x1d9 *eurus_cmd_0x1d9;
	struct ps3_eurus_cmd_ap_opmode *eurus_cmd_ap_opmode;
	struct ps3_eurus_cmd_ap_ssid *eurus_cmd_ap_ssid;
	struct ps3_eurus_cmd_ap_wep_config *eurus_cmd_ap_wep_config;
	struct ps3_eurus_cmd_0x61 *eurus_cmd_0x61;
	struct ps3_eurus_cmd_0x65 *eurus_cmd_0x65;
	struct ps3_eurus_cmd_0xc5 *eurus_cmd_0xc5;
	struct ps3_eurus_cmd_0x127 *eurus_cmd_0x127;
	struct ps3_eurus_cmd_0x12b *eurus_cmd_0x12b;
	struct ps3_eurus_cmd_ap_wpa_akm_suite *eurus_cmd_ap_wpa_akm_suite;
	struct ps3_eurus_cmd_ap_wpa_group_cipher_suite *eurus_cmd_ap_wpa_group_cipher_suite;
	struct ps3_eurus_cmd_ap_wpa_pairwise_cipher_suite *eurus_cmd_ap_wpa_pairwise_cipher_suite;
	struct ps3_eurus_cmd_ap_wpa_psk_passphrase *eurus_cmd_ap_wpa_psk_passphrase;
	struct ps3_eurus_cmd_ap_wpa_psk_bin *eurus_cmd_ap_wpa_psk_bin;
	struct ps3_eurus_cmd_0xd5 *eurus_cmd_0xd5;
	struct ps3_eurus_cmd_0x1 *eurus_cmd_0x1;
	struct ps3_eurus_cmd_start_ap *eurus_cmd_start_ap;
	unsigned char *buf = NULL;
	unsigned int status, wep_key_max_length;
	int i, err;

	buf = kmalloc(PS3_JUPITER_AP_CMD_BUFSIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	/* set channel */

	eurus_cmd_set_channel = (struct ps3_eurus_cmd_set_channel *) buf;
        memset(eurus_cmd_set_channel, 0, sizeof(*eurus_cmd_set_channel));
        eurus_cmd_set_channel->channel = japd->channel;

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_SET_CHANNEL,
	    eurus_cmd_set_channel, sizeof(*eurus_cmd_set_channel), &status, NULL, NULL);
	if (err)
		goto done;

	if (status != PS3_EURUS_CMD_OK) {
		err = -EIO;
		goto done;
	}

	eurus_cmd_0x1d9 = (struct ps3_eurus_cmd_0x1d9 *) buf;
        memset(eurus_cmd_0x1d9, 0, sizeof(*eurus_cmd_0x1d9));
	eurus_cmd_0x1d9->unknown1 = 0x1;

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x1d9,
	    eurus_cmd_0x1d9, sizeof(*eurus_cmd_0x1d9), &status, NULL, NULL);
	if (err)
		goto done;

	if (status != PS3_EURUS_CMD_OK) {
		err = -EIO;
		goto done;
	}

	/* set opmode */

	eurus_cmd_ap_opmode = (struct ps3_eurus_cmd_ap_opmode *) buf;
        memset(eurus_cmd_ap_opmode, 0, sizeof(*eurus_cmd_ap_opmode));

	switch (japd->opmode) {
	case PS3_JUPITER_AP_OPMODE_11B:
		eurus_cmd_ap_opmode->opmode = cpu_to_le32(PS3_EURUS_AP_OPMODE_11B);
	break;
	case PS3_JUPITER_AP_OPMODE_11G:
		eurus_cmd_ap_opmode->opmode = cpu_to_le32(PS3_EURUS_AP_OPMODE_11G);
	break;
	case PS3_JUPITER_AP_OPMODE_11BG:
		eurus_cmd_ap_opmode->opmode = cpu_to_le32(PS3_EURUS_AP_OPMODE_11BG);
	break;
	}

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_SET_AP_OPMODE,
	    eurus_cmd_ap_opmode, sizeof(*eurus_cmd_ap_opmode), &status, NULL, NULL);
	if (err)
		goto done;

	if (status != PS3_EURUS_CMD_OK) {
		err = -EIO;
		goto done;
	}

	/* set ssid */

	eurus_cmd_ap_ssid = (struct ps3_eurus_cmd_ap_ssid *) buf;
        memset(eurus_cmd_ap_ssid, 0, sizeof(*eurus_cmd_ap_ssid));
	memcpy(eurus_cmd_ap_ssid->ssid, japd->essid, japd->essid_length);

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_SET_AP_SSID,
	    eurus_cmd_ap_ssid, sizeof(*eurus_cmd_ap_ssid), &status, NULL, NULL);
	if (err)
		goto done;

	if (status != PS3_EURUS_CMD_OK) {
		err = -EIO;
		goto done;
	}

	if (japd->wpa_mode == PS3_JUPITER_AP_WPA_MODE_NONE) {
		/* set WEP configuration */

		eurus_cmd_ap_wep_config = (struct ps3_eurus_cmd_ap_wep_config *) buf;
        	memset(eurus_cmd_ap_wep_config, 0, sizeof(*eurus_cmd_ap_wep_config));

		wep_key_max_length = 0;

		for (i = 0; i < PS3_JUPITER_AP_WEP_KEYS; i++) {
			pr_debug("%s: key#%d length %d\n", __func__, i, japd->key_length[i]);

			if (japd->key_config_status & (1 << i)) {
				if (japd->key_length[i] > wep_key_max_length)
					wep_key_max_length = japd->key_length[i];
			}

			memcpy(eurus_cmd_ap_wep_config->key[i], japd->key, japd->key_length[i]);
		}

		if (wep_key_max_length == 0)
			eurus_cmd_ap_wep_config->security_mode = PS3_EURUS_WEP_SECURITY_NONE;
		else if (wep_key_max_length <= 5)
			eurus_cmd_ap_wep_config->security_mode = PS3_EURUS_WEP_SECURITY_40BIT;
		else
			eurus_cmd_ap_wep_config->security_mode = PS3_EURUS_WEP_SECURITY_104BIT;

		err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_SET_AP_WEP_CONFIG,
		    eurus_cmd_ap_wep_config, sizeof(*eurus_cmd_ap_wep_config), &status, NULL, NULL);
		if (err)
			goto done;

		if (status != PS3_EURUS_CMD_OK) {
			err = -EIO;
			goto done;
		}

		eurus_cmd_0x61 = (struct ps3_eurus_cmd_0x61 *) buf;
        	memset(eurus_cmd_0x61, 0, sizeof(*eurus_cmd_0x61));
		eurus_cmd_0x61->unknown = wep_key_max_length > 0 ? 0x2 : 0x0;

		err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x61,
		    eurus_cmd_0x61, sizeof(*eurus_cmd_0x61), &status, NULL, NULL);
		if (err)
			goto done;

		if (status != PS3_EURUS_CMD_OK) {
			err = -EIO;
			goto done;
		}

		if (wep_key_max_length > 0) {
			eurus_cmd_0x65 = (struct ps3_eurus_cmd_0x65 *) buf;
        		memset(eurus_cmd_0x65, 0, sizeof(*eurus_cmd_0x65));
			eurus_cmd_0x65->unknown = 0x1;

			err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x65,
			    eurus_cmd_0x65, sizeof(*eurus_cmd_0x65), &status, NULL, NULL);
			if (err)
				goto done;

			if (status != PS3_EURUS_CMD_OK) {
				err = -EIO;
				goto done;
			}
		} else {
			eurus_cmd_0xc5 = (struct ps3_eurus_cmd_0xc5 *) buf;
        		memset(eurus_cmd_0xc5, 0, sizeof(*eurus_cmd_0xc5));

			err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0xc5,
			    eurus_cmd_0xc5, sizeof(*eurus_cmd_0xc5), &status, NULL, NULL);
			if (err)
				goto done;

			if (status != PS3_EURUS_CMD_OK) {
				err = -EIO;
				goto done;
			}
		}
	} else {
		/* set WPA configuration */

		eurus_cmd_0xc5 = (struct ps3_eurus_cmd_0xc5 *) buf;
        	memset(eurus_cmd_0xc5, 0, sizeof(*eurus_cmd_0xc5));
		eurus_cmd_0xc5->unknown = cpu_to_le32(1);

		err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0xc5,
		    eurus_cmd_0xc5, sizeof(*eurus_cmd_0xc5), &status, NULL, NULL);
		if (err)
			goto done;

		if (status != PS3_EURUS_CMD_OK) {
			err = -EIO;
			goto done;
		}

		eurus_cmd_0x65 = (struct ps3_eurus_cmd_0x65 *) buf;
        	memset(eurus_cmd_0x65, 0, sizeof(*eurus_cmd_0x65));
		eurus_cmd_0x65->unknown = 0x1;

		err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x65,
		    eurus_cmd_0x65, sizeof(*eurus_cmd_0x65), &status, NULL, NULL);
		if (err)
			goto done;

		if (status != PS3_EURUS_CMD_OK) {
			err = -EIO;
			goto done;
		}

		eurus_cmd_0x61 = (struct ps3_eurus_cmd_0x61 *) buf;
        	memset(eurus_cmd_0x61, 0, sizeof(*eurus_cmd_0x61));

		err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x61,
		    eurus_cmd_0x61, sizeof(*eurus_cmd_0x61), &status, NULL, NULL);
		if (err)
			goto done;

		if (status != PS3_EURUS_CMD_OK) {
			err = -EIO;
			goto done;
		}

		eurus_cmd_0x127 = (struct ps3_eurus_cmd_0x127 *) buf;
        	memset(eurus_cmd_0x127, 0, sizeof(*eurus_cmd_0x127));

		err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x127,
		    eurus_cmd_0x127, sizeof(*eurus_cmd_0x127), &status, NULL, NULL);
		if (err)
			goto done;

		if (status != PS3_EURUS_CMD_OK) {
			err = -EIO;
			goto done;
		}

		eurus_cmd_0x12b = (struct ps3_eurus_cmd_0x12b *) buf;
        	memset(eurus_cmd_0x12b, 0, sizeof(*eurus_cmd_0x12b));

		err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x12b,
		    eurus_cmd_0x12b, sizeof(*eurus_cmd_0x12b), &status, NULL, NULL);
		if (err)
			goto done;

		if (status != PS3_EURUS_CMD_OK) {
			err = -EIO;
			goto done;
		}

		/* set WPA group cipher */

		eurus_cmd_ap_wpa_group_cipher_suite = (struct ps3_eurus_cmd_ap_wpa_group_cipher_suite *) buf;
        	memset(eurus_cmd_ap_wpa_group_cipher_suite, 0, sizeof(*eurus_cmd_ap_wpa_group_cipher_suite));

		switch (japd->group_cipher_mode) {
		case PS3_JUPITER_AP_CIPHER_TKIP:
			if (japd->wpa_mode == PS3_JUPITER_AP_WPA_MODE_WPA)
				eurus_cmd_ap_wpa_group_cipher_suite->cipher_suite = PS3_EURUS_WPA_CIPHER_SUITE_WPA_TKIP;
			else
				eurus_cmd_ap_wpa_group_cipher_suite->cipher_suite = PS3_EURUS_WPA_CIPHER_SUITE_WPA2_TKIP;
		break;
		case PS3_JUPITER_AP_CIPHER_AES:
			if (japd->wpa_mode == PS3_JUPITER_AP_WPA_MODE_WPA)
				eurus_cmd_ap_wpa_group_cipher_suite->cipher_suite = PS3_EURUS_WPA_CIPHER_SUITE_WPA_AES;
			else
				eurus_cmd_ap_wpa_group_cipher_suite->cipher_suite = PS3_EURUS_WPA_CIPHER_SUITE_WPA2_AES;
		break;
		default:
			/* should never occur */
			BUG();
		}

		err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_SET_AP_WPA_GROUP_CIPHER_SUITE,
		    eurus_cmd_ap_wpa_group_cipher_suite, sizeof(*eurus_cmd_ap_wpa_group_cipher_suite), &status, NULL, NULL);
		if (err)
			goto done;

		if (status != PS3_EURUS_CMD_OK) {
			err = -EIO;
			goto done;
		}

		/* set WPA pairwise cipher */

		eurus_cmd_ap_wpa_pairwise_cipher_suite = (struct ps3_eurus_cmd_ap_wpa_pairwise_cipher_suite *) buf;
        	memset(eurus_cmd_ap_wpa_pairwise_cipher_suite, 0, sizeof(*eurus_cmd_ap_wpa_pairwise_cipher_suite));
		eurus_cmd_ap_wpa_pairwise_cipher_suite->unknown = 1;

		switch (japd->pairwise_cipher_mode) {
		case PS3_JUPITER_AP_CIPHER_TKIP:
			if (japd->wpa_mode == PS3_JUPITER_AP_WPA_MODE_WPA)
				eurus_cmd_ap_wpa_pairwise_cipher_suite->cipher_suite = PS3_EURUS_WPA_CIPHER_SUITE_WPA_TKIP;
			else
				eurus_cmd_ap_wpa_pairwise_cipher_suite->cipher_suite = PS3_EURUS_WPA_CIPHER_SUITE_WPA2_TKIP;
		break;
		case PS3_JUPITER_AP_CIPHER_AES:
			if (japd->wpa_mode == PS3_JUPITER_AP_WPA_MODE_WPA)
				eurus_cmd_ap_wpa_pairwise_cipher_suite->cipher_suite = PS3_EURUS_WPA_CIPHER_SUITE_WPA_AES;
			else
				eurus_cmd_ap_wpa_pairwise_cipher_suite->cipher_suite = PS3_EURUS_WPA_CIPHER_SUITE_WPA2_AES;
		break;
		default:
			/* should never occur */
			BUG();
		}

		err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_SET_AP_WPA_PAIRWISE_CIPHER_SUITE,
		    eurus_cmd_ap_wpa_pairwise_cipher_suite, sizeof(*eurus_cmd_ap_wpa_pairwise_cipher_suite), &status, NULL, NULL);
		if (err)
			goto done;

		if (status != PS3_EURUS_CMD_OK) {
			err = -EIO;
			goto done;
		}

		/* set WPA mode */

		eurus_cmd_ap_wpa_akm_suite = (struct ps3_eurus_cmd_ap_wpa_akm_suite *) buf;
        	memset(eurus_cmd_ap_wpa_akm_suite, 0, sizeof(*eurus_cmd_ap_wpa_akm_suite));
		eurus_cmd_ap_wpa_akm_suite->unknown = 1;

		switch (japd->wpa_mode) {
		case PS3_JUPITER_AP_WPA_MODE_WPA:
			eurus_cmd_ap_wpa_akm_suite->suite = PS3_EURUS_WPA_AKM_SUITE_WPA_PSK;
		break;
		case PS3_JUPITER_AP_WPA_MODE_WPA2:
			eurus_cmd_ap_wpa_akm_suite->suite = PS3_EURUS_WPA_AKM_SUITE_WPA2_PSK;
		break;
		default:
			/* should never occur */ ;
		}

		err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_SET_AP_WPA_AKM_SUITE,
		    eurus_cmd_ap_wpa_akm_suite, sizeof(*eurus_cmd_ap_wpa_akm_suite), &status, NULL, NULL);
		if (err)
			goto done;

		if (status != PS3_EURUS_CMD_OK) {
			err = -EIO;
			goto done;
		}

		/* set WPA PSK passphrase */

		if (japd->psk_passphrase) {
			eurus_cmd_ap_wpa_psk_passphrase = (struct ps3_eurus_cmd_ap_wpa_psk_passphrase *) buf;
        		memset(eurus_cmd_ap_wpa_psk_passphrase, 0, sizeof(*eurus_cmd_ap_wpa_psk_passphrase));
			memcpy(eurus_cmd_ap_wpa_psk_passphrase->passphrase, japd->psk, japd->psk_length);

			err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_SET_AP_WPA_PSK_PASSPHRASE,
			    eurus_cmd_ap_wpa_psk_passphrase, sizeof(*eurus_cmd_ap_wpa_psk_passphrase), &status, NULL, NULL);
			if (err)
				goto done;

			if (status != PS3_EURUS_CMD_OK) {
				err = -EIO;
				goto done;
			}
		}

		/* set WPA PSK binary */

		eurus_cmd_ap_wpa_psk_bin = (struct ps3_eurus_cmd_ap_wpa_psk_bin *) buf;
        	memset(eurus_cmd_ap_wpa_psk_bin, 0, sizeof(*eurus_cmd_ap_wpa_psk_bin));

		if (!japd->psk_passphrase) {
			eurus_cmd_ap_wpa_psk_bin->enable = 1;
			memcpy(eurus_cmd_ap_wpa_psk_bin->psk, japd->psk, japd->psk_length);
		}

		err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_SET_AP_WPA_PSK_BIN,
		    eurus_cmd_ap_wpa_psk_bin, sizeof(*eurus_cmd_ap_wpa_psk_bin), &status, NULL, NULL);
		if (err)
			goto done;

		if (status != PS3_EURUS_CMD_OK) {
			err = -EIO;
			goto done;
		}

		eurus_cmd_0xd5 = (struct ps3_eurus_cmd_0xd5 *) buf;
        	memset(eurus_cmd_0xd5, 0, sizeof(*eurus_cmd_0xd5));
		eurus_cmd_0xd5->unknown = cpu_to_le32(36000);

		err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0xd5,
		    eurus_cmd_0xd5, sizeof(*eurus_cmd_0xd5), &status, NULL, NULL);
		if (err)
			goto done;

		if (status != PS3_EURUS_CMD_OK) {
			err = -EIO;
			goto done;
		}
	}

	eurus_cmd_0x1 = (struct ps3_eurus_cmd_0x1 *) buf;
        memset(eurus_cmd_0x1, 0, sizeof(*eurus_cmd_0x1));
	eurus_cmd_0x1->unknown = 0x1;

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_0x1,
	    eurus_cmd_0x1, sizeof(*eurus_cmd_0x1), &status, NULL, NULL);
	if (err)
		goto done;

	if (status != PS3_EURUS_CMD_OK) {
		err = -EIO;
		goto done;
	}

	/* start AP */

	init_completion(&japd->start_ap_done_comp);

	japd->start_ap_status = PS3_JUPITER_AP_START_IN_PROGRESS;

	eurus_cmd_start_ap = (struct ps3_eurus_cmd_start_ap *) buf;
	memset(eurus_cmd_start_ap, 0, sizeof(*eurus_cmd_start_ap));
	eurus_cmd_start_ap->unknown = 0x2;

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_START_AP,
	    eurus_cmd_start_ap, sizeof(*eurus_cmd_start_ap), &status, NULL, NULL);
	if (err)
		goto done;

	if (status != PS3_EURUS_CMD_OK) {
		err = -EIO;
		goto done;
	}

	err = wait_for_completion_timeout(&japd->start_ap_done_comp, 5 * HZ);
	if (!err) {
		/* timeout */
		ps3_jupiter_ap_stop_ap(japd);
		err = -EIO;
		goto done;
	}

	japd->start_ap_status = PS3_JUPITER_AP_STARTED;

	err = 0;

done:

	if (err)
		japd->start_ap_status = PS3_JUPITER_AP_STOPPED;

	kfree(buf);

	return err;
}

/*
 * ps3_jupiter_ap_start_ap_worker
 */
static void ps3_jupiter_ap_start_ap_worker(struct work_struct *work)
{
	struct ps3_jupiter_ap_dev *japd = container_of(work, struct ps3_jupiter_ap_dev, start_ap_work.work);
	struct usb_device *udev = japd->udev;
	int err;

	mutex_lock(&japd->start_ap_lock);

	if (japd->start_ap_status != PS3_JUPITER_AP_STOPPED) {
		mutex_unlock(&japd->start_ap_lock);
		return;
	}

	dev_dbg(&udev->dev, "starting AP\n");

	err = ps3_jupiter_ap_config_and_start_ap(japd);
	if (err) {
		dev_dbg(&udev->dev, "AP start failed (%d)\n", err);
		goto done;
	}

done:

	mutex_unlock(&japd->start_ap_lock);
}

/*
 * ps3_jupiter_ap_start_ap
 */
static void ps3_jupiter_ap_start_ap(struct ps3_jupiter_ap_dev *japd)
{
	pr_debug("%s: called\n", __func__);

	if (!test_bit(PS3_JUPITER_AP_READY, &japd->status))
		return;

	if (!test_bit(PS3_JUPITER_AP_CONFIG_ESSID_SET, &japd->config_status))
		return;

	if ((japd->wpa_mode == PS3_JUPITER_AP_WPA_MODE_NONE) &&
	    (japd->group_cipher_mode == PS3_JUPITER_AP_CIPHER_WEP) &&
	    !japd->key_config_status)
		return;

	if ((japd->wpa_mode != PS3_JUPITER_AP_WPA_MODE_NONE) &&
	    !test_bit(PS3_JUPITER_AP_CONFIG_WPA_PSK_SET, &japd->config_status))
		return;

	queue_delayed_work(japd->start_ap_queue, &japd->start_ap_work, 0);

	pr_debug("%s: done\n", __func__);
}

/*
 * ps3_jupiter_ap_stop_ap
 */
static int ps3_jupiter_ap_stop_ap(struct ps3_jupiter_ap_dev *japd)
{
	struct ps3_eurus_cmd_start_ap *eurus_cmd_start_ap;
	unsigned char *buf = NULL;
	unsigned int status;
	int err;

	buf = kmalloc(PS3_JUPITER_AP_CMD_BUFSIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	eurus_cmd_start_ap = (struct ps3_eurus_cmd_start_ap *) buf;
	memset(eurus_cmd_start_ap, 0, sizeof(*eurus_cmd_start_ap));
	eurus_cmd_start_ap->unknown = 0x1;

	err = ps3_jupiter_exec_eurus_cmd(PS3_EURUS_CMD_START_AP,
	    eurus_cmd_start_ap, sizeof(*eurus_cmd_start_ap), &status, NULL, NULL);
	if (err)
		goto done;

	if (status != PS3_EURUS_CMD_OK) {
		err = -EIO;
		goto done;
	}

	err = 0;

done:

	kfree(buf);

	return err;
}

/*
 * ps3_jupiter_ap_create_start_ap_worker
 */
static int ps3_jupiter_ap_create_start_ap_worker(struct ps3_jupiter_ap_dev *japd)
{
	japd->start_ap_queue = create_singlethread_workqueue("ps3_jupiter_ap_start");
	if (!japd->start_ap_queue)
		return -ENOMEM;

	INIT_DELAYED_WORK(&japd->start_ap_work, ps3_jupiter_ap_start_ap_worker);

	return 0;
}

/*
 * ps3_jupiter_ap_destroy_start_ap_worker
 */
static void ps3_jupiter_ap_destroy_start_ap_worker(struct ps3_jupiter_ap_dev *japd)
{
	if (japd->start_ap_queue) {
		cancel_delayed_work(&japd->start_ap_work);
		flush_workqueue(japd->start_ap_queue);
		destroy_workqueue(japd->start_ap_queue);
		japd->start_ap_queue = NULL;
	}
}

/*
 * ps3_jupiter_ap_reset_state
 */
static void ps3_jupiter_ap_reset_state(struct ps3_jupiter_ap_dev *japd)
{
	japd->config_status = 0;

	japd->opmode = PS3_JUPITER_AP_OPMODE_11G;

	japd->auth_mode = PS3_JUPITER_AP_AUTH_OPEN;

	japd->wpa_mode = PS3_JUPITER_AP_WPA_MODE_NONE;
	japd->group_cipher_mode = PS3_JUPITER_AP_CIPHER_NONE;
	japd->pairwise_cipher_mode = PS3_JUPITER_AP_CIPHER_NONE;

	memset(japd->essid, 0, sizeof(japd->essid));
	japd->essid_length = 0;

	japd->channel = 0;

	japd->key_config_status = 0;
	japd->curr_key_index = 0;

	japd->psk_length = 0;
	japd->psk_passphrase = 0;

	japd->start_ap_status = PS3_JUPITER_AP_STOPPED;
}

/*
 * ps3_jupiter_ap_probe
 */
static int ps3_jupiter_ap_probe(struct usb_interface *interface,
	const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(interface);
	struct ps3_jupiter_ap_dev *japd;
	struct net_device *netdev;
	int err;

	netdev = alloc_etherdev(sizeof(struct ps3_jupiter_ap_dev));
	if (!netdev)
		return -ENOMEM;

	japd = netdev_priv(netdev);
	japd->netdev = netdev;

	SET_NETDEV_DEV(netdev, &udev->dev);

	strcpy(netdev->name, "wlan%d");

	netdev->ethtool_ops = &ps3_jupiter_ap_ethtool_ops;
	netdev->netdev_ops = &ps3_jupiter_ap_net_device_ops;
	netdev->wireless_data = &japd->wireless_data;
	netdev->wireless_handlers = &ps3_jupiter_ap_iw_handler_def;

	japd->udev = usb_get_dev(udev);
	usb_set_intfdata(interface, japd);

	err = ps3_jupiter_ap_set_mac_addr(japd);
	if (err) {
		dev_err(&udev->dev, "could not setup network device (%d)\n", err);
		goto fail_free_netdev;
	}

	spin_lock_init(&japd->lock);

	japd->event_listener.notifier_call = ps3_jupiter_ap_event_handler;

	err = ps3_jupiter_register_event_listener(&japd->event_listener);
	if (err) {
		dev_err(&udev->dev, "could not register event listener (%d)\n", err);
		goto fail_free_netdev;
	}

	err = ps3_jupiter_ap_get_channel_info(japd);
	if (err) {
		dev_err(&udev->dev, "could not get channel info (%d)\n", err);
		goto fail_unregister_event_listener;
	}

	mutex_init(&japd->start_ap_lock);

	err = ps3_jupiter_ap_create_start_ap_worker(japd);
	if (err) {
		dev_err(&udev->dev, "could not create start AP work queue (%d)\n", err);
		goto fail_unregister_event_listener;
	}

	skb_queue_head_init(&japd->rx_skb_queue);
	tasklet_init(&japd->rx_tasklet, ps3_jupiter_ap_rx_tasklet, (unsigned long) japd);

	err = ps3_jupiter_ap_alloc_rx_urbs(japd);
	if (err) {
		dev_err(&udev->dev, "could not allocate Rx URBs (%d)\n", err);
		goto fail_destroy_start_ap_worker;
	}

	init_usb_anchor(&japd->tx_urb_anchor);
	atomic_set(&japd->tx_submitted_urbs, 0);

	ps3_jupiter_ap_reset_state(japd);

	set_bit(PS3_JUPITER_AP_READY, &japd->status);

	err = register_netdev(netdev);
	if (err) {
		dev_dbg(&udev->dev, "could not register network device %s (%d)\n", netdev->name, err);
		goto fail_free_rx_urbs;
	}

	return 0;

fail_free_rx_urbs:

	ps3_jupiter_ap_free_rx_urbs(japd);

fail_destroy_start_ap_worker:

	ps3_jupiter_ap_destroy_start_ap_worker(japd);

fail_unregister_event_listener:

	ps3_jupiter_unregister_event_listener(&japd->event_listener);

fail_free_netdev:

	usb_set_intfdata(interface, NULL);
	usb_put_dev(udev);

	free_netdev(netdev);

	return err;
}

/*
 * ps3_jupiter_ap_disconnect
 */
static void ps3_jupiter_ap_disconnect(struct usb_interface *interface)
{
	struct ps3_jupiter_ap_dev *japd = usb_get_intfdata(interface);
	struct usb_device *udev = japd->udev;
	struct net_device *netdev = japd->netdev;

	clear_bit(PS3_JUPITER_AP_READY, &japd->status);

	unregister_netdev(netdev);

	if (japd->start_ap_status == PS3_JUPITER_AP_STARTED)
		ps3_jupiter_ap_stop_ap(japd);

	ps3_jupiter_ap_destroy_start_ap_worker(japd);

	ps3_jupiter_ap_free_rx_urbs(japd);
	tasklet_kill(&japd->rx_tasklet);
	ps3_jupiter_ap_purge_rx_skb_queue(japd);

	ps3_jupiter_ap_free_tx_urbs(japd);

	ps3_jupiter_unregister_event_listener(&japd->event_listener);

	usb_set_intfdata(interface, NULL);
	usb_put_dev(udev);

	free_netdev(netdev);
}

#ifdef CONFIG_PM
/*
 * ps3_jupiter_ap_suspend
 */
static int ps3_jupiter_ap_suspend(struct usb_interface *interface, pm_message_t state)
{
	/* XXX: implement */

	return 0;
}

/*
 * ps3_jupiter_ap_resume
 */
static int ps3_jupiter_ap_resume(struct usb_interface *interface)
{
	/* XXX: implement */

	return 0;
}
#endif /* CONFIG_PM */

static struct usb_device_id ps3_jupiter_ap_devtab[] = {
	{
		.match_flags = USB_DEVICE_ID_MATCH_VENDOR | USB_DEVICE_ID_MATCH_INT_INFO,
		.idVendor = 0x054c,
		.idProduct = 0x036f,
		.bInterfaceClass = USB_CLASS_VENDOR_SPEC,
		.bInterfaceSubClass = 2,
		.bInterfaceProtocol = 3
	},
	{ }
};

static struct usb_driver ps3_jupiter_ap_drv = {
	.name		= KBUILD_MODNAME,
	.id_table	= ps3_jupiter_ap_devtab,
	.probe		= ps3_jupiter_ap_probe,
	.disconnect	= ps3_jupiter_ap_disconnect,
#ifdef CONFIG_PM
	.suspend	= ps3_jupiter_ap_suspend,
	.resume		= ps3_jupiter_ap_resume,
#endif /* CONFIG_PM */
};

/*
 * ps3_jupiter_ap_init
 */
static int __init ps3_jupiter_ap_init(void)
{
	return usb_register(&ps3_jupiter_ap_drv);
}

/*
 * ps3_jupiter_ap_exit
 */
static void __exit ps3_jupiter_ap_exit(void)
{
	usb_deregister(&ps3_jupiter_ap_drv);
}

module_init(ps3_jupiter_ap_init);
module_exit(ps3_jupiter_ap_exit);

MODULE_SUPPORTED_DEVICE("PS3 Jupiter AP");
MODULE_DEVICE_TABLE(usb, ps3_jupiter_ap_devtab);
MODULE_DESCRIPTION("PS3 Jupiter AP");
MODULE_AUTHOR("glevand");
MODULE_LICENSE("GPL");
