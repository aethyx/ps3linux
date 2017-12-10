/*-
 * Copyright (C) 2012 glevand <geoffrey.levand@mail.ru>
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>

#include <openssl/aes.h>

static struct option long_opts[] = {
	{ "device",		no_argument, NULL, 'd' },
	{ "verbose",		no_argument, NULL, 'v' },
	{ "rnd1",		no_argument, NULL, 'r' },
	{ "rnd2",		no_argument, NULL, 's' },
	{ "rnd3",		no_argument, NULL, 't' },
	{ "key1",		no_argument, NULL, 'k' },
	{ "key2",		no_argument, NULL, 'l' },
	{ "magic",		no_argument, NULL, 'm' },
	{ "atakeyindex",	no_argument, NULL, 'i' },
	{ "atakey",		no_argument, NULL, 'a' },
	{ NULL, 0, NULL, 0 }
};

static const char *device = "/dev/ps3encdec";
static int verbose;
static unsigned char rnd1[AES_BLOCK_SIZE];
static unsigned char rnd2[2 * AES_BLOCK_SIZE];
static unsigned char rnd3[AES_BLOCK_SIZE];
static unsigned char key1[2 * AES_BLOCK_SIZE] = {
	0xe3, 0xf2, 0x26, 0x65, 0xaf, 0xc4, 0xe1, 0xc0,
	0x14, 0xa4, 0x31, 0x24, 0x1d, 0xbc, 0x0b, 0x69,
	0xd5, 0xd6, 0x68, 0x57, 0xd9, 0x1e, 0x6b, 0x27,
};
static unsigned char key2[2 * AES_BLOCK_SIZE] = {
	0x66, 0x86, 0x6a, 0xf7, 0x48, 0x9a, 0xe8, 0x5a,
	0xbf, 0x98, 0xa6, 0x70, 0xaa, 0x27, 0x67, 0x2e,
	0x06, 0x6e, 0x60, 0xd1, 0x4d, 0x52, 0x41, 0x21,
};
static unsigned char magic[2 * AES_BLOCK_SIZE] = {
	0xeb, 0x97, 0x06, 0xb9, 0xa7, 0x5a, 0x48, 0x85,
	0x3b, 0xd4, 0x03, 0x5a, 0xde, 0x93, 0x6e, 0x05,
	0x0e, 0x87, 0xe7, 0x42, 0xd2, 0x7a, 0x86, 0x09,
};
static int atakeyindex = 1;
static unsigned char atakey[2 * AES_BLOCK_SIZE];
static int atakey_length;

static int
parse_hex(const char *s, unsigned char *b, int maxlen)
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

static int
parse_opts(int argc, char **argv)
{
	int c;
	char *endptr;

	while ((c = getopt_long(argc, argv, "d:vr:s:k:t:l:m:i:a:", long_opts, NULL)) != -1) {
		switch (c) {
		case 'd':
			device = optarg;
		break;
		case 'v':
			verbose++;
		break;
		case 'r':
			if (parse_hex(optarg, rnd1, AES_BLOCK_SIZE) != AES_BLOCK_SIZE) {
				fprintf(stderr, "invalid rnd1 specified: %s\n", optarg);
				return (-1);
			}
		break;
		case 's':
			if (parse_hex(optarg, rnd2, 2 * AES_BLOCK_SIZE) != 2 * AES_BLOCK_SIZE) {
				fprintf(stderr, "invalid rnd2 specified: %s\n", optarg);
				return (-1);
			}
		break;
		case 't':
			if (parse_hex(optarg, rnd3, AES_BLOCK_SIZE) != AES_BLOCK_SIZE) {
				fprintf(stderr, "invalid rnd3 specified: %s\n", optarg);
				return (-1);
			}
		break;
		case 'k':
			if (parse_hex(optarg, key1, 2 * AES_BLOCK_SIZE) != 24) {
				fprintf(stderr, "invalid key1 specified: %s\n", optarg);
				return (-1);
			}
		break;
		case 'l':
			if (parse_hex(optarg, key2, 2 * AES_BLOCK_SIZE) != 24) {
				fprintf(stderr, "invalid key2 specified: %s\n", optarg);
				return (-1);
			}
		break;
		case 'm':
			if (parse_hex(optarg, magic, 2 * AES_BLOCK_SIZE) != 24) {
				fprintf(stderr, "invalid magic specified: %s\n", optarg);
				return (-1);
			}
		break;
		case 'i':
			atakeyindex = strtol(optarg, &endptr, 0);
			if (*endptr != '\0') {
				fprintf(stderr, "invalid atakeyindex specified: %s\n", optarg);
				return (-1);
			}
		break;
		case 'a':
			atakey_length = parse_hex(optarg, atakey, 2 * AES_BLOCK_SIZE);
			if ((atakey_length != 16) && (atakey_length != 24)) {
				fprintf(stderr, "invalid atakey specified: %s\n", optarg);
				return (-1);
			}
		break;
		default:
			fprintf(stderr, "invalid option specified: %c\n", c);
			return (-1);
		break;
		}
	}

	return (0);
}

static void
aes_cbc_encrypt(const unsigned char *iv, const unsigned char *key, int key_length,
	const unsigned char *data, int data_length, unsigned char *out)
{
	AES_KEY aes_key;
	unsigned char cbc[AES_BLOCK_SIZE];
	int i;

	AES_set_encrypt_key(key, key_length, &aes_key);

	memcpy(cbc, iv, AES_BLOCK_SIZE);

	while (data_length >= AES_BLOCK_SIZE) {
		for (i = 0; i < AES_BLOCK_SIZE; i++)
			out[i] = cbc[i] ^ data[i];

		AES_encrypt(out, out, &aes_key);

		memcpy(cbc, out, AES_BLOCK_SIZE);

		data += AES_BLOCK_SIZE;
		out += AES_BLOCK_SIZE;
		data_length -= AES_BLOCK_SIZE;
	}
}

static void
aes_cbc_decrypt(const unsigned char *iv, const unsigned char *key, int key_length,
	const unsigned char *data, int data_length, unsigned char *out)
{
	AES_KEY aes_key;
	unsigned char cbc[AES_BLOCK_SIZE];
	unsigned char buf[AES_BLOCK_SIZE];
	int i;

	AES_set_decrypt_key(key, key_length, &aes_key);

	memcpy(cbc, iv, AES_BLOCK_SIZE);

	while (data_length >= AES_BLOCK_SIZE) {
		memcpy(buf, data, AES_BLOCK_SIZE);

		AES_decrypt(data, out, &aes_key);

		for (i = 0; i < AES_BLOCK_SIZE; i++)
			out[i] ^= cbc[i];

		memcpy(cbc, buf, AES_BLOCK_SIZE);

		data += AES_BLOCK_SIZE;
		out += AES_BLOCK_SIZE;
		data_length -= AES_BLOCK_SIZE;
	}
}

static void
hex_fprintf(FILE *fp, const unsigned char *buf, size_t len)
{
	int i;

	if (len <= 0)
		return;

	for (i = 0; i < len; i++) {
		if ((i > 0) && !(i % 16))
			fprintf(fp, "\n");

		fprintf(fp, "%02x ", buf[i]);
	}

	fprintf(fp, "\n");
}

static int
read_timeout(int fd, void *buf, size_t len, unsigned int timeout)
{
	fd_set fds;
	struct timeval tv;
	int ret;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	tv.tv_sec = timeout / 1000000;
	tv.tv_usec = timeout % 1000000;

	ret = select(fd + 1, &fds, NULL, NULL, &tv);
	if (ret <= 0)
		return (-1);

	return (read(fd, buf, len));
}

static int
kgen_flash(const char *device)
{
	int fd;
	unsigned char buf[4];
	int ret;

	fd = open(device, O_RDWR | O_NONBLOCK);
	if (fd < 0)
		return (-1);

	memset(buf, 0, sizeof(buf));
	*(uint32_t *) buf = 0x84;

	ret = write(fd, buf, 4);
	if (ret < 0) {
		close(fd);
		return (-1);
	}

	close(fd);

	return (0);
}

static int
kgen1(const char *device, const unsigned char *iv, const unsigned char *rnd,
	const unsigned char *key, unsigned char *out)
{
	int fd;
	unsigned char buf[0x50];
	uint16_t status;
	int ret;

	fd = open(device, O_RDWR | O_NONBLOCK);
	if (fd < 0)
		return (-1);

	memset(buf, 0, sizeof(buf));
	*(uint32_t *) buf = 0x81;
	*(uint16_t *) (buf + 4) = 0x1;
	*(uint16_t *) (buf + 6) = 0x30;
	memcpy(buf + 8, iv, 0x10);

	aes_cbc_encrypt(iv, key, 8 * 24, rnd, 0x20, buf + 0x18);

	if (verbose)
		hex_fprintf(stdout, buf, 0x44);

	ret = write(fd, buf, 0x44);
	if (ret < 0) {
		close(fd);
		return (-1);
	}

	ret = read_timeout(fd, out, 0x50, 1000000);
	if (ret < 0) {
		close(fd);
		return (-1);
	}

	close(fd);

	status = *(uint16_t *) out;
	if (status != 0x2)
		return (-2);

	return (0);
}

static int
kgen2(const char *device, const unsigned char *iv, const unsigned char *rnd,
	const unsigned char *key, unsigned char *out)
{
	int fd;
	unsigned char buf[0x34];
	uint16_t status;
	int ret;

	fd = open(device, O_RDWR | O_NONBLOCK);
	if (fd < 0)
		return (-1);

	memset(buf, 0, sizeof(buf));
	*(uint32_t *) buf = 0x82;
	*(uint16_t *) (buf + 4) = 0x3;
	*(uint16_t *) (buf + 6) = 0x20;

	aes_cbc_encrypt(iv, key, 8 * 24, rnd, 0x20, buf + 0x8);

	if (verbose)
		hex_fprintf(stdout, buf, 0x34);

	ret = write(fd, buf, 0x34);
	if (ret < 0) {
		close(fd);
		return (-1);
	}

	ret = read_timeout(fd, out, 2, 1000000);
	if (ret < 0) {
		close(fd);
		return (-1);
	}

	close(fd);

	status = *(uint16_t *) out;
	if (status != 0x4)
		return (-2);

	return (0);
}

static int
kset(const char *device, const unsigned char *iv, const unsigned char *key,
	const unsigned char *in, unsigned char *out)
{
	int fd;
	unsigned char buf[0x64];
	uint16_t status;
	int ret;

	fd = open(device, O_RDWR | O_NONBLOCK);
	if (fd < 0)
		return (-1);

	memset(buf, 0, sizeof(buf));
	*(uint32_t *) buf = 0x83;
	*(uint16_t *) (buf + 4) = 0x1;
	*(uint16_t *) (buf + 6) = 0x50;
	memcpy(buf + 8, iv, 0x10);

	aes_cbc_encrypt(iv, key, 8 * 24, in, 0x40, buf + 0x18);

	if (verbose)
		hex_fprintf(stdout, buf, 0x64);

	ret = write(fd, buf, 0x64);
	if (ret < 0) {
		close(fd);
		return (-1);
	}

	ret = read_timeout(fd, out, 0x10, 1000000);
	if (ret < 0) {
		close(fd);
		return (-1);
	}

	close(fd);

	status = *(uint16_t *) out;
	if (status != 0x2)
		return (-2);

	return (0);
}

static int
sb_clear(const char *device, unsigned int arg)
{
	int fd;
	unsigned char buf[0x8];
	int ret;

	fd = open(device, O_RDWR | O_NONBLOCK);
	if (fd < 0)
		return (-1);

	memset(buf, 0, sizeof(buf));
	*(uint32_t *) buf = 0x87;
	*(uint32_t *) (buf + 4) = arg;

	if (verbose)
		hex_fprintf(stdout, buf, 0x8);

	ret = write(fd, buf, 0x8);
	if (ret < 0) {
		close(fd);
		return (-1);
	}

	close(fd);

	return (0);
}

int
main(int argc, char **argv)
{
	unsigned char buf[256];
	unsigned char rnd4[2 * AES_BLOCK_SIZE];
	unsigned char key3[2 * AES_BLOCK_SIZE];
	int i;
	int ret;

	ret = parse_opts(argc, argv);
	if (ret)
		exit(255);

	fprintf(stdout, "rnd1: ");
	hex_fprintf(stdout, rnd1, 0x10);

	fprintf(stdout, "rnd2: ");
	hex_fprintf(stdout, rnd2, 0x20);

	fprintf(stdout, "rnd3: ");
	hex_fprintf(stdout, rnd3, 0x10);

	fprintf(stdout, "magic: ");
	hex_fprintf(stdout, magic, 0x18);

	fprintf(stdout, "atakey: ");
	hex_fprintf(stdout, atakey, 0x18);

	ret = kgen1(device, rnd1, rnd2, key1, buf);
	if (ret) {
		fprintf(stderr, "could not kgen1\n");
		goto failed;
	}

	if (verbose)
		hex_fprintf(stdout, buf, 0x50);

	aes_cbc_decrypt(rnd1, key2, 8 * 24, buf + 0x4, 0x20, buf + 0x4);

	if (memcmp(rnd2, buf + 0x4, 0x20)) {
		fprintf(stderr, "rnd2 mismatch\n");
		goto failed;
	}

	aes_cbc_decrypt(rnd1, key2, 8 * 24, buf + 0x24, 0x20, rnd4);

	fprintf(stdout, "rnd4: ");
	hex_fprintf(stdout, rnd4, 0x20);

	ret = kgen2(device, rnd1, rnd4, key1, buf);
	if (ret) {
		fprintf(stderr, "could not kgen2\n");
		goto failed;
	}

	if (verbose)
		hex_fprintf(stdout, buf, 0x2);

	for (i = 0; i < 0x20; i++)
		key3[i] = rnd2[i] ^ rnd4[i];

	fprintf(stdout, "key3: ");
	hex_fprintf(stdout, key3, 0x20);

	memset(buf, 0, 0x40);
	memcpy(buf, magic, 0x18);
	buf[0x18 + 0] = 0x0;
	buf[0x18 + 1] = 0x0;
	buf[0x18 + 2] = (atakeyindex >= 0x100) ? 0x2 : 0x3;
	buf[0x18 + 3] = atakeyindex & 0xf;
	buf[0x18 + 7] = (atakeyindex >= 0x110) ? 0x0 : 0x1;
	memcpy(buf + 0x20, atakey, atakey_length);

	if (verbose)
		hex_fprintf(stdout, buf, 0x40);

	ret = kset(device, rnd3, key3, buf, buf);
	if (ret) {
		fprintf(stderr, "could not kset\n");
		goto failed;
	}

	if (verbose)
		hex_fprintf(stdout, buf, 0x10);

	if (atakeyindex >= 0x110) {
		ret = sb_clear(device, (atakeyindex & 0xf) + 1);
		if (ret) {
			fprintf(stderr, "could not sb clear\n");
			goto failed;
		}
	}

	exit(0);

failed:

	kgen_flash(device);

	exit(1);
}
