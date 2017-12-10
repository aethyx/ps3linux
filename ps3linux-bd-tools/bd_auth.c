/*-
 * Copyright (C) 2012-2013 glevand <geoffrey.levand@mail.ru>
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

#include <openssl/aes.h>
#include <openssl/des.h>

#include "bd_util.h"

static const unsigned char iv1[AES_BLOCK_SIZE] = {
	0x22, 0x26, 0x92, 0x8d, 0x44, 0x03, 0x2f, 0x43, 0x6a, 0xfd, 0x26, 0x7e, 0x74, 0x8b, 0x23, 0x93,
};

static const unsigned char iv2[DES_KEY_SZ] = {
	0xe8, 0x0b, 0x3f, 0x0c, 0xd6, 0x56, 0x6d, 0xd0,
};

static const unsigned char iv3[AES_BLOCK_SIZE] = {
	0x3b, 0xd6, 0x24, 0x02, 0x0b, 0xd3, 0xf8, 0x65, 0xe8, 0x0b, 0x3f, 0x0c, 0xd6, 0x56, 0x6d, 0xd0,
};

static const unsigned char key3[AES_BLOCK_SIZE] = {
	0x12, 0x6c, 0x6b, 0x59, 0x45, 0x37, 0x0e, 0xee, 0xca, 0x68, 0x26, 0x2d, 0x02, 0xdd, 0x12, 0xd2,
};

static const unsigned char key4[AES_BLOCK_SIZE] = {
	0xd9, 0xa2, 0x0a, 0x79, 0x66, 0x6c, 0x27, 0xd1, 0x10, 0x32, 0xac, 0xcf, 0x0d, 0x7f, 0xb5, 0x01,
};

static const unsigned char key5[AES_BLOCK_SIZE] = {
	0x19, 0x76, 0x6f, 0xbc, 0x77, 0xe4, 0xe7, 0x5c, 0xf4, 0x41, 0xe4, 0x8b, 0x94, 0x2c, 0x5b, 0xd9,
};

static const unsigned char key6[AES_BLOCK_SIZE] = {
	0x50, 0xcb, 0xa7, 0xf0, 0xc2, 0xa7, 0xc0, 0xf6, 0xf3, 0x3a, 0x21, 0x43, 0x26, 0xac, 0x4e, 0xf3,
};

static const unsigned char cmd_4_14[] = {
	0x1e, 0x79, 0x18, 0x8e, 0x09, 0x3b, 0xc8, 0x77, 0x95, 0xb2, 0xcf, 0x2a, 0xe7, 0xaf, 0x9b, 0xb4,
	0x86, 0x80, 0x18, 0x28, 0xc2, 0xca, 0x05, 0xba, 0xd1, 0xf2, 0x78, 0xf1, 0x80, 0x1f, 0xea, 0xcb,
	0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x8d, 0xb3, 0x46, 0x93, 0x42, 0x64, 0x81, 0x60, 0x16, 0x8f, 0x51, 0xd1, 0x93, 0x76, 0x23, 0x95,
};

static struct option long_opts[] = {
	{ "device",	no_argument, NULL, 'd' },
	{ "verbose",	no_argument, NULL, 'v' },
	{ "rnd1",	no_argument, NULL, 'r' },
	{ "key1",	no_argument, NULL, 'k' },
	{ "key2",	no_argument, NULL, 'l' },
	{ NULL, 0, NULL, 0 }
};

static const char *device = "/dev/sr0";
static int verbose;
static unsigned char rnd1[AES_BLOCK_SIZE] = {
	0x06, 0xd7, 0x33, 0xcb, 0x22, 0x4a, 0x83, 0x56, 0xa3, 0xe8, 0x39, 0x78, 0x66, 0xe4, 0x3e, 0xc2,
};
static unsigned char key1[AES_BLOCK_SIZE];
static int key1_length;
static unsigned char key2[AES_BLOCK_SIZE];
static int key2_length;

static int
parse_opts(int argc, char **argv)
{
	int c;

	while ((c = getopt_long(argc, argv, "d:vtr:s:k:l:", long_opts, NULL)) != -1) {
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
		case 'k':
			key1_length = parse_hex(optarg, key1, 2 * AES_BLOCK_SIZE);
			if (key1_length != AES_BLOCK_SIZE) {
				fprintf(stderr, "invalid key1 specified: %s\n", optarg);
				return (-1);
			}
		break;
		case 'l':
			key2_length = parse_hex(optarg, key2, 2 * AES_BLOCK_SIZE);
			if (key2_length != AES_BLOCK_SIZE) {
				fprintf(stderr, "invalid key2 specified: %s\n", optarg);
				return (-1);
			}
		break;
		default:
			fprintf(stderr, "invalid option specified: %c\n", c);
			return (-1);
		break;
		}
	}

	if (key1_length <= 0) {
		fprintf(stderr, "no key1 specified\n");
		return (-1);
	}

	if (key2_length <= 0) {
		fprintf(stderr, "no key2 specified\n");
		return (-1);
	}

	return (0);
}

static unsigned char
cksum(const unsigned char *data, int len)
{
	unsigned short sum = 0;

	while (len--)
		sum += *data++;

	return (~sum);
}

int
main(int argc, char **argv)
{
	unsigned char cdb[256];
	unsigned char buf[256];
	unsigned char rnd2[AES_BLOCK_SIZE];
	unsigned char key7[AES_BLOCK_SIZE];
	unsigned char key8[AES_BLOCK_SIZE];
	unsigned int req_sense;
	int ret;

	ret = parse_opts(argc, argv);
	if (ret)
		exit(255);

	/* test unit ready */

	if (verbose)
		fprintf(stdout, "=== TEST UNIT READY (0x00) ===\n");

	ret = test_unit_ready(device, &req_sense);
	if (ret && (req_sense != 0x23a00)) {
		fprintf(stderr, "TEST UNIT READY failed: req sense %x\n", req_sense);
		exit(1);
	}

	/* security check */

	if (verbose)
		fprintf(stdout, "=== SEND KEY (0xA3) ===\n");

	memset(buf, 0, 0x14);

	ret = send_key(device, 0xe0, 0x14, 0x0, 0x0, buf, &req_sense);
	if (ret) {
		fprintf(stderr, "SEND KEY failed: req sense %x\n", req_sense);
		exit(1);
	}

	/* establish session */

	memset(buf, 0, 0x14);
	*(uint16_t *) buf = htobe16(0x10);
	memcpy(buf + 4, rnd1, sizeof(rnd1));

	fprintf(stdout, "rnd1: ");
	hex_fprintf(stdout, rnd1, sizeof(rnd1));

	if (verbose)
		hex_fprintf(stdout, buf, 0x14);

	aes_cbc_encrypt(iv1, key1, 8 * sizeof(key1), buf + 4, 0x10, buf + 4);

	if (verbose)
		hex_fprintf(stdout, buf, 0x14);

	/* send encrypted host random */

	if (verbose)
		fprintf(stdout, "=== SEND KEY (0xA3) ===\n");

	ret = send_key(device, 0xe0, 0x14, 0x0, 0x0, buf, &req_sense);
	if (ret) {
		fprintf(stderr, "SEND KEY failed: req sense %x\n", req_sense);
		exit(1);
	}

	/* receive encrypted host and drive randoms */

	if (verbose)
		fprintf(stdout, "=== REPORT KEY (0xA4) ===\n");

	ret = report_key(device, 0xe0, 0x24, 0x0, 0x0, buf, &req_sense);
	if (ret) {
		fprintf(stderr, "REPORT KEY failed: req sense %x\n", req_sense);
		exit(1);
	}

	if (verbose)
		hex_fprintf(stdout, buf, 0x24);

	/* NB: decrypt received host and drive randoms separately */

	aes_cbc_decrypt(iv1, key2, 8 * sizeof(key2), buf + 4, 0x10, buf + 4);
	aes_cbc_decrypt(iv1, key2, 8 * sizeof(key2), buf + 0x14, 0x10, buf + 0x14);

	if (verbose)
		hex_fprintf(stdout, buf, 0x24);

	/* verify received host random */

	if (memcmp(rnd1, buf + 4, sizeof(rnd1))) {
		fprintf(stderr, "rnd1 mismatch\n");
		exit(1);
	}

	memcpy(rnd2, buf + 0x14, sizeof(rnd2));

	fprintf(stdout, "rnd2: ");
	hex_fprintf(stdout, rnd2, sizeof(rnd2));

	memset(buf, 0, 0x14);
	*(uint16_t *) buf = htobe16(0x10);
	memcpy(buf + 4, rnd2, sizeof(rnd2));

	if (verbose)
		hex_fprintf(stdout, buf, 0x14);

	aes_cbc_encrypt(iv1, key1, 8 * sizeof(key1), buf + 4, 0x10, buf + 4);

	if (verbose)
		hex_fprintf(stdout, buf, 0x14);

	/* send encrypted drive random */

	if (verbose)
		fprintf(stdout, "=== SEND KEY (0xA3) ===\n");

	ret = send_key(device, 0xe0, 0x14, 0x0, 0x2, buf, &req_sense);
	if (ret) {
		fprintf(stderr, "SEND KEY failed: req sense %x\n", req_sense);
		exit(1);
	}

	/* derive session keys from host and drive randoms */

	memcpy(key7, rnd1, 8);
	memcpy(key7 + 8, rnd2 + 8, 8);

	if (verbose)
		hex_fprintf(stdout, key7, sizeof(key7));

	aes_cbc_encrypt(iv1, key3, 8 * sizeof(key3), key7, sizeof(key7), key7);

	fprintf(stdout, "key7: ");
	hex_fprintf(stdout, key7, sizeof(key7));

	memcpy(key8, rnd1 + 8, 8);
	memcpy(key8 + 8, rnd2, 8);

	if (verbose)
		hex_fprintf(stdout, key8, sizeof(key8));

	aes_cbc_encrypt(iv1, key4, 8 * sizeof(key4), key8, sizeof(key8), key8);

	fprintf(stdout, "key8: ");
	hex_fprintf(stdout, key8, sizeof(key8));

	if (verbose)
		fprintf(stdout, "=== UNKNOWN (0xE1) ===\n");

	memset(cdb, 0, 8);
	cdb[6] = 0xe6;					/* random byte */
	cdb[7] = cksum(cdb, 7);

	if (verbose)
		hex_fprintf(stdout, cdb, 8);

	triple_des_cbc_encrypt(iv2, key7, cdb, 8, cdb);

	if (verbose)
		hex_fprintf(stdout, cdb, 8);

	memset(buf, 0, 0x54);
	*(uint16_t *) buf = htobe16(0x50);
	buf[5] = 0xee;					/* random byte */
	memcpy(buf + 8, cmd_4_14, sizeof(cmd_4_14));
	buf[4] = cksum(buf + 5, 0x4f);

	if (verbose)
		hex_fprintf(stdout, buf, 0x54);

	aes_cbc_encrypt(iv3, key7, 8 * sizeof(key7), buf + 4, 0x50, buf + 4);

	if (verbose)
		hex_fprintf(stdout, buf, 0x54);

	ret = send_e1(device, 0x54, cdb, buf, &req_sense);
	if (ret) {
		fprintf(stderr, "SEND E1 failed: req sense %x\n", req_sense);
		exit(1);
	}

	/* establish session again */

	memset(buf, 0, 0x14);
	*(uint16_t *) buf = htobe16(0x10);
	memcpy(buf + 4, rnd1, sizeof(rnd1));

	fprintf(stdout, "rnd1: ");
	hex_fprintf(stdout, rnd1, sizeof(rnd1));

	if (verbose)
		hex_fprintf(stdout, buf, 0x14);

	aes_cbc_encrypt(iv1, key5, 8 * sizeof(key5), buf + 4, 0x10, buf + 4);

	if (verbose)
		hex_fprintf(stdout, buf, 0x14);

	/* send encrypted host random */

	if (verbose)
		fprintf(stdout, "=== SEND KEY (0xA3) ===\n");

	ret = send_key(device, 0xe0, 0x14, 0x0, 0x1, buf, &req_sense);
	if (ret) {
		fprintf(stderr, "SEND KEY failed: req sense %x\n", req_sense);
		exit(1);
	}

	/* receive encrypted host and drive randoms */

	if (verbose)
		fprintf(stdout, "=== REPORT KEY (0xA4) ===\n");

	ret = report_key(device, 0xe0, 0x24, 0x0, 0x1, buf, &req_sense);
	if (ret) {
		fprintf(stderr, "REPORT KEY failed: req sense %x\n", req_sense);
		exit(1);
	}

	if (verbose)
		hex_fprintf(stdout, buf, 0x24);

	/* NB: decrypt received host and drive randoms separately */

	aes_cbc_decrypt(iv1, key6, 8 * sizeof(key6), buf + 4, 0x10, buf + 4);
	aes_cbc_decrypt(iv1, key6, 8 * sizeof(key6), buf + 0x14, 0x10, buf + 0x14);

	if (verbose)
		hex_fprintf(stdout, buf, 0x24);

	/* verify received host random */

	if (memcmp(rnd1, buf + 4, sizeof(rnd1))) {
		fprintf(stderr, "rnd1 mismatch\n");
		exit(1);
	}

	memcpy(rnd2, buf + 0x14, sizeof(rnd2));

	fprintf(stdout, "rnd2: ");
	hex_fprintf(stdout, rnd2, sizeof(rnd2));

	memset(buf, 0, 0x14);
	*(uint16_t *) buf = htobe16(0x10);
	memcpy(buf + 4, rnd2, sizeof(rnd2));

	if (verbose)
		hex_fprintf(stdout, buf, 0x14);

	aes_cbc_encrypt(iv1, key5, 8 * sizeof(key5), buf + 4, 0x10, buf + 4);

	if (verbose)
		hex_fprintf(stdout, buf, 0x14);

	/* send encrypted drive random */

	if (verbose)
		fprintf(stdout, "=== SEND KEY (0xA3) ===\n");

	ret = send_key(device, 0xe0, 0x14, 0x0, 0x3, buf, &req_sense);
	if (ret) {
		fprintf(stderr, "SEND KEY failed: req sense %x\n", req_sense);
		exit(1);
	}

	/* derive session keys from host and drive randoms */

	memcpy(key7, rnd1, 8);
	memcpy(key7 + 8, rnd2 + 8, 8);

	if (verbose)
		hex_fprintf(stdout, key7, sizeof(key7));

	aes_cbc_encrypt(iv1, key3, 8 * sizeof(key3), key7, sizeof(key7), key7);

	fprintf(stdout, "key7: ");
	hex_fprintf(stdout, key7, sizeof(key7));

	memcpy(key8, rnd1 + 8, 8);
	memcpy(key8 + 8, rnd2, 8);

	if (verbose)
		hex_fprintf(stdout, key8, sizeof(key8));

	aes_cbc_encrypt(iv1, key4, 8 * sizeof(key4), key8, sizeof(key8), key8);

	fprintf(stdout, "key8: ");
	hex_fprintf(stdout, key8, sizeof(key8));

	/* retrieve response */

	if (verbose)
		fprintf(stdout, "=== UNKNOWN (0xE0) ===\n");

	memset(cdb, 0, 8);
	cdb[0] = 0x4;					/* random byte */
	cdb[6] = 0xe7;					/* random byte */
	cdb[7] = cksum(cdb, 7);

	if (verbose)
		hex_fprintf(stdout, cdb, 8);

	triple_des_cbc_encrypt(iv2, key7, cdb, 8, cdb);

	if (verbose)
		hex_fprintf(stdout, cdb, 8);

	ret = send_e0(device, 0x54, cdb, buf, &req_sense);
	if (ret) {
		fprintf(stderr, "SEND E0 failed: req sense %x\n", req_sense);
		exit(1);
	}

	if (verbose)
		hex_fprintf(stdout, buf, 0x54);

	aes_cbc_decrypt(iv3, key7, 8 * sizeof(key7), buf + 4, 0x50, buf + 4);

	if (verbose)
		hex_fprintf(stdout, buf, 0x54);

	if (buf[4] != cksum(buf + 5, 0x4f)) {
		fprintf(stderr, "checksum mismatch\n");
		exit(1);
	}

	fprintf(stdout, "version: ");
	hex_fprintf(stdout, buf + 6, 8);

	exit(0);
}
