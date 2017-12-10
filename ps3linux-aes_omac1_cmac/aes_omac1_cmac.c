/*
 * AES OMAC1 CMAC
 *
 * Copyright (C) 2003-2007, Jouni Malinen <j@w1.fi>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <unistd.h>

#include <openssl/aes.h>

static struct option long_opts[] = {
	{ "key",		no_argument, NULL, 'k' },
	{ NULL, 0, NULL, 0 }
};

static unsigned char key[2 * AES_BLOCK_SIZE];
static int key_length;

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

	while ((c = getopt_long(argc, argv, "k:", long_opts, NULL)) != -1) {
		switch (c) {
		case 'k':
			key_length = parse_hex(optarg, key, 2 * AES_BLOCK_SIZE);
			if ((key_length != 16) && (key_length != 24) && (key_length != 32)) {
				fprintf(stderr, "invalid key specified: %s\n", optarg);
				return (-1);
			}
		break;
		default:
			fprintf(stderr, "invalid option specified: %c\n", c);
			return (-1);
		break;
		}
	}

	if (key_length <= 0) {
		fprintf(stderr, "no key specified\n");
		return (-1);
	}

	return (0);
}

/*
 * gf_mulx
 */
static void gf_mulx(unsigned char *pad)
{
        int carry;
	int i;

        carry = pad[0] & 0x80;

        for (i = 0; i < AES_BLOCK_SIZE - 1; i++)
                pad[i] = (pad[i] << 1) | (pad[i + 1] >> 7);

        pad[AES_BLOCK_SIZE - 1] <<= 1;

        if (carry)
                pad[AES_BLOCK_SIZE - 1] ^= 0x87;
}

/*
 * aes_omac1_cmac
 */
static void aes_omac1_cmac(const unsigned char *key, int key_length,
	const unsigned char *data, int data_length, unsigned char *mac)
{
	AES_KEY aes_key;
	unsigned char cbc[AES_BLOCK_SIZE];
	unsigned char pad[AES_BLOCK_SIZE];
	int i;

	AES_set_encrypt_key(key, key_length, &aes_key);

	memset(cbc, 0, AES_BLOCK_SIZE);

	while (data_length >= AES_BLOCK_SIZE) {
		for (i = 0; i < AES_BLOCK_SIZE; i++)
			cbc[i] ^= *data++;

		if (data_length > AES_BLOCK_SIZE)
			AES_encrypt(cbc, cbc, &aes_key);

		data_length -= AES_BLOCK_SIZE;
	}

	memset(pad, 0, AES_BLOCK_SIZE);
	AES_encrypt(pad, pad, &aes_key);
	gf_mulx(pad);

	if (data_length) {
		for (i = 0; i < data_length; i++)
			cbc[i] ^= *data++;

		cbc[data_length] ^= 0x80;
		gf_mulx(pad);
			
	}

	for (i = 0; i < AES_BLOCK_SIZE; i++)
		pad[i] ^= cbc[i];

	AES_encrypt(pad, mac, &aes_key);
}

/*
 * main
 */
int main(int argc, char **argv)
{
	unsigned char buf[4096];
	unsigned char *data = NULL;
	unsigned int data_length = 0;
	unsigned char mac[AES_BLOCK_SIZE];
	int nread;
	int err;

	err = parse_opts(argc, argv);
	if (err)
		return (1);

	while (!feof(stdin)) {
		nread = fread(buf, 1, sizeof(buf), stdin);
		if (nread <= 0)
			break;

		data = realloc(data, data_length + nread);
		if (!data)
			return (1);

		memcpy(data + data_length, buf, nread);
		data_length += nread;
	}

	aes_omac1_cmac(key, 8 * key_length, data, data_length, mac);

	fwrite(mac, 1, sizeof(mac), stdout);

	return (0);
}
