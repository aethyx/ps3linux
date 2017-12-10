/*
 * DES CBC
 *
 * Copyright (C) 2013 glevand <geoffrey.levand@mail.ru>
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

#include <openssl/des.h>

static struct option long_opts[] = {
	{ "decrypt",		no_argument, NULL, 'd' },
	{ "iv",			no_argument, NULL, 'i' },
	{ "key",		no_argument, NULL, 'k' },
	{ NULL, 0, NULL, 0 }
};

static int decrypt;
static unsigned char iv[DES_KEY_SZ];
static int iv_length;
static unsigned char key[DES_KEY_SZ];
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

	while ((c = getopt_long(argc, argv, "di:k:", long_opts, NULL)) != -1) {
		switch (c) {
		case 'd':
			decrypt++;
		break;
		case 'i':
			iv_length = parse_hex(optarg, iv, DES_KEY_SZ);
			if (iv_length != DES_KEY_SZ) {
				fprintf(stderr, "invalid iv specified: %s\n", optarg);
				return (-1);
			}
		break;
		case 'k':
			key_length = parse_hex(optarg, key, DES_KEY_SZ);
			if (key_length != DES_KEY_SZ) {
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

	if (iv_length <= 0) {
		fprintf(stderr, "no iv specified\n");
		return (-1);
	}

	if (key_length <= 0) {
		fprintf(stderr, "no key specified\n");
		return (-1);
	}

	return (0);
}

static void
_des_cbc_encrypt(const unsigned char *iv, const unsigned char *key,
	const unsigned char *data, int data_length, unsigned char *out)
{
	DES_cblock kcb;
	DES_cblock ivcb;
	DES_key_schedule ks;

	memcpy(kcb, key, DES_KEY_SZ);
	memcpy(ivcb, iv, DES_KEY_SZ);

	DES_set_key_unchecked(&kcb, &ks);

	DES_cbc_encrypt(data, out, data_length, &ks, &ivcb, DES_ENCRYPT);
}

static void
_des_cbc_decrypt(const unsigned char *iv, const unsigned char *key,
	const unsigned char *data, int data_length, unsigned char *out)
{
	DES_cblock kcb;
	DES_cblock ivcb;
	DES_key_schedule ks;

	memcpy(kcb, key, DES_KEY_SZ);
	memcpy(ivcb, iv, DES_KEY_SZ);

	DES_set_key_unchecked(&kcb, &ks);

	DES_cbc_encrypt(data, out, data_length, &ks, &ivcb, DES_DECRYPT);
}

/*
 * main
 */
int main(int argc, char **argv)
{
	unsigned char buf[4096];
	unsigned char *data = NULL;
	unsigned int data_length = 0;
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

	if (decrypt) {
		_des_cbc_decrypt(iv, key, data, data_length, data);
	} else {
		_des_cbc_encrypt(iv, key, data, data_length, data);
	}

	fwrite(data, 1, data_length, stdout);

	return (0);
}
