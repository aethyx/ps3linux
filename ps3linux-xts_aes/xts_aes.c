/*
 * XTS AES
 *
 * Copyright (C) 2012 glevand <geoffrey.levand@mail.ru>
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
#include <string.h>
#include <ctype.h>
#include <getopt.h>

#include <openssl/aes.h>

static struct option long_opts[] = {
	{ "decrypt",		no_argument, NULL, 'd' },
	{ "tweakkey",		no_argument, NULL, 't' },
	{ "datakey",		no_argument, NULL, 'k' },
	{ "sector",		no_argument, NULL, 's' },
	{ "reverse",		no_argument, NULL, 'r' },
	{ NULL, 0, NULL, 0 }
};

static int decrypt;
static unsigned char tweak_key[2 * AES_BLOCK_SIZE];
static int tweak_key_length;
static unsigned char data_key[2 * AES_BLOCK_SIZE];
static int data_key_length;
static unsigned int sector;
static int reverse;

/*
 * parse_hex
 */
static int parse_hex(const char *s, unsigned char *b, int maxlen)
{
	int len;
	char c;
	int i;

	len = strlen(s);
	if (len % 2)
		return (-1);

	for (i = 0; (i < (len / 2)) && (i < maxlen); i++) {
		if (!isxdigit(s[2 * i + 0]) || !isxdigit(s[2 * i + 1]))
			return (-1);

		b[i] = 0;

		c = tolower(s[2 * i + 0]);
		if (isdigit(c))
			b[i] += (c - '0') * 16;
		else
			b[i] += (c - 'a' + 10) * 16;

		c = tolower(s[2 * i + 1]);
		if (isdigit(c))
			b[i] += c - '0';
		else
			b[i] += c - 'a' + 10;
	}

	return (i);
}

/*
 * parse_opts
 */
static int parse_opts(int argc, char **argv)
{
	int c;
	char *endptr;

	while ((c = getopt_long(argc, argv, "dt:k:s:r", long_opts, NULL)) != -1) {
		switch (c) {
		case 'd':
			decrypt++;
		break;
		case 't':
			tweak_key_length = parse_hex(optarg, tweak_key, 2 * AES_BLOCK_SIZE);
			if ((tweak_key_length != 16) && (tweak_key_length != 24) && (tweak_key_length != 32)) {
				fprintf(stderr, "invalid tweak key specified: %s\n", optarg);
				return (-1);
			}
		break;
		case 'k':
			data_key_length = parse_hex(optarg, data_key, 2 * AES_BLOCK_SIZE);
			if ((data_key_length != 16) && (data_key_length != 24) && (data_key_length != 32)) {
				fprintf(stderr, "invalid data key specified: %s\n", optarg);
				return (-1);
			}
		break;
		case 's':
			sector = strtoul(optarg, &endptr, 0);
			if (*endptr != '\0') {
				fprintf(stderr, "invalid sector specified: %s\n", optarg);
				return (-1);
			}
		break;
		case 'r':
			reverse++;
		break;
		default:
			fprintf(stderr, "invalid option specified: %c\n", c);
			return (-1);
		break;
		}
	}

	if (tweak_key_length <= 0) {
		fprintf(stderr, "no tweak key specified\n");
		return (-1);
	}

	if (data_key_length <= 0) {
		fprintf(stderr, "no data key specified\n");
		return (-1);
	}

	return (0);
}

/*
 * xts_aes_encrypt
 */
static void xts_aes_encrypt(const unsigned char *key1, int key1_length,
	const unsigned char *key2, int key2_length,
	unsigned long long sector, int reverse,
	const unsigned char *data, int data_length, unsigned char *out)
{
	AES_KEY aes_key1;
	AES_KEY aes_key2;
	unsigned char tweak[AES_BLOCK_SIZE];
	unsigned char tmp, cin, cout;
	int i;

	AES_set_encrypt_key(key1, key1_length, &aes_key1);
	AES_set_encrypt_key(key2, key2_length, &aes_key2);

	for (i = 0; i < AES_BLOCK_SIZE; i++) {
		tweak[i] = sector & 0xff;
		sector >>= 8;
	}

	AES_encrypt(tweak, tweak, &aes_key2);

	while (data_length >= AES_BLOCK_SIZE) {
		for (i = 0; i < AES_BLOCK_SIZE; i++)
			out[i] = tweak[i] ^ data[i];

		AES_encrypt(out, out, &aes_key1);

		for (i = 0; i < AES_BLOCK_SIZE; i++)
			out[i] = tweak[i] ^ out[i];

		if (reverse) {
			for (i = 0; i < AES_BLOCK_SIZE; i += 2) {
				tmp = out[i];
				out[i] = out[i + 1];
				out[i + 1] = tmp;
			}
		}

		cin = 0;

		for (i = 0; i < AES_BLOCK_SIZE; i++) {
			cout = (tweak[i] >> 7) & 0x1;
			tweak[i] = ((tweak[i] << 1) + cin) & 0xff;
			cin = cout;
		}

		if (cout)
			tweak[0] ^= 0x87;

		data += AES_BLOCK_SIZE;
		out += AES_BLOCK_SIZE;
		data_length -= AES_BLOCK_SIZE;
	}
}

/*
 * xts_aes_decrypt
 */
static void xts_aes_decrypt(const unsigned char *key1, int key1_length,
	const unsigned char *key2, int key2_length,
	unsigned long long sector, int reverse,
	const unsigned char *data, int data_length, unsigned char *out)
{
	AES_KEY aes_key1;
	AES_KEY aes_key2;
	unsigned char tweak[AES_BLOCK_SIZE];
	unsigned char cin, cout;
	int i;

	AES_set_decrypt_key(key1, key1_length, &aes_key1);
	AES_set_encrypt_key(key2, key2_length, &aes_key2);

	for (i = 0; i < AES_BLOCK_SIZE; i++) {
		tweak[i] = sector & 0xff;
		sector >>= 8;
	}

	AES_encrypt(tweak, tweak, &aes_key2);

	while (data_length >= AES_BLOCK_SIZE) {
		for (i = 0; i < AES_BLOCK_SIZE; i += 2) {
			if (reverse) {
				out[i] = data[i + 1];
				out[i + 1] = data[i];
			} else {
				out[i] = data[i];
				out[i + 1] = data[i + 1];
			}
		}

		for (i = 0; i < AES_BLOCK_SIZE; i++)
			out[i] = tweak[i] ^ out[i];

		AES_decrypt(out, out, &aes_key1);

		for (i = 0; i < AES_BLOCK_SIZE; i++)
			out[i] = tweak[i] ^ out[i];

		cin = 0;

		for (i = 0; i < AES_BLOCK_SIZE; i++) {
			cout = (tweak[i] >> 7) & 0x1;
			tweak[i] = ((tweak[i] << 1) + cin) & 0xff;
			cin = cout;
		}

		if (cout)
			tweak[0] ^= 0x87;

		data += AES_BLOCK_SIZE;
		out += AES_BLOCK_SIZE;
		data_length -= AES_BLOCK_SIZE;
	}
}

/*
 * main
 */
int main(int argc, char **argv)
{
	unsigned char in[512];
	unsigned char out[512];
	int nread;
	int err;

	err = parse_opts(argc, argv);
	if (err)
		return (1);

	while (!feof(stdin)) {
		nread = fread(in, 1, 512, stdin);
		if (nread < 512)
			break;

		if (decrypt) {
			xts_aes_decrypt(data_key, 8 * data_key_length, tweak_key, 8 * tweak_key_length,
				sector, reverse, in, 512, out);
		} else {
			xts_aes_encrypt(data_key, 8 * data_key_length, tweak_key, 8 * tweak_key_length,
				sector, reverse, in, 512, out);
		}

		fwrite(out, 1, 512, stdout);

		sector++;
	}

	return (0);
}
