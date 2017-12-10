/*-
 * Copyright (C) 2013 glevand <geoffrey.levand@mail.ru>
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
#include <ctype.h>
#include <getopt.h>
#include <endian.h>

#include <unistd.h>

#include "bd_util.h"

#define BUFFER_ID			0
#define MAX_WRITE_BUFFER_LENGTH		0x8000

static struct option long_opts[] = {
	{ "device",	no_argument, NULL, 'd' },
	{ "verbose",	no_argument, NULL, 'v' },
	{ "h_id",	no_argument, NULL, 'h' },
	{ "firmware",	no_argument, NULL, 'f' },
	{ "timeout",	no_argument, NULL, 't' },
	{ NULL, 0, NULL, 0 }
};

static const struct {
	const char *id;
	uint64_t type;
} id_table[] = {
	{ "PS-SYSTEM   300R", 0x1200000000000001 },
	{ "PS-SYSTEM   301R", 0x1200000000000002 },
	{ "PS-SYSTEM   302R", 0x1200000000000003 },
	{ "PS-SYSTEM   303R", 0x1200000000000004 },
	{ "PS-SYSTEM   304R", 0x1200000000000005 },
	{ "PS-SYSTEM   306R", 0x1200000000000007 },
	{ "PS-SYSTEM   308R", 0x1200000000000008 },
	{ "PS-SYSTEM   310R", 0x1200000000000009 },
	{ "PS-SYSTEM   312R", 0x120000000000000a },
	{ "PS-SYSTEM   314R", 0x120000000000000b },
};

static const struct {
	unsigned int req_sense;
	const char *message;
} req_sense_table[] = {
	{ 0x20407, "because of invalid command issued. ignore it" },
	{ 0x23a00, "success" },
	{ 0x43e01, "failure erasing or writing flash of BD drive" },
	{ 0x52400, "invalid data length or continuous error" },
	{ 0x52600, "invalid firmware combination or hash error" },
};

static const char *device = "/dev/sr0";
static int verbose;
static uint64_t h_id;
static char *firmware_path;
static int timeout_sec = 60;

static int
parse_opts(int argc, char **argv)
{
	int c;
	char *endptr;

	while ((c = getopt_long(argc, argv, "d:vh:f:t:", long_opts, NULL)) != -1) {
		switch (c) {
		case 'd':
			device = optarg;
		break;
		case 'v':
			verbose++;
		break;
		case 'h':
			h_id = strtoull(optarg, &endptr, 0);
			if ((*endptr != '\0') || (h_id == 0)) {
				fprintf(stderr, "invalid h_id specified: %s\n", optarg);
				exit(1);
			}
		break;
		case 'f':
			firmware_path = optarg;
		break;
		case 't':
			timeout_sec = strtol(optarg, &endptr, 0);
			if (*endptr != '\0') {
				fprintf(stderr, "invalid timeout specified: %s\n", optarg);
				exit(1);
			}
		break;
		default:
			fprintf(stderr, "invalid option specified: %c\n", c);
			return (-1);
		break;
		}
	}

	if (firmware_path == NULL) {
		fprintf(stderr, "no firmware specified\n");
		return (-1);
	}

	return (0);
}

static const char *
req_sense_message(unsigned int req_sense)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(req_sense_table); i++) {
		if (req_sense_table[i].req_sense == req_sense)
			return req_sense_table[i].message;
	}

	return ("unknown");
}

int
main(int argc, char **argv)
{
	unsigned char data[256];
	unsigned char *firmware;
	int firmware_len;
	char vendor_id[9], product_id[17];
	unsigned int offset;
	unsigned int req_sense;
	unsigned int timeout;
	int i, nwrite, ret;

	ret = parse_opts(argc, argv);
	if (ret)
		exit(255);

	/* read firmware */

	firmware = read_file(firmware_path, &firmware_len);
	if (firmware == NULL) {
		fprintf(stderr, "invalid firmware specified\n");
		exit(1);
	}

	fprintf(stdout, "firmware length %d\n", firmware_len);
	fprintf(stdout, "firmware h_id %llx\n", h_id);

	/* inquiry */

	if (verbose)
		fprintf(stdout, "=== INQUIRY (0x12) ===\n");

	ret = inquiry(device, data, 36, &req_sense);
	if (ret) {
		fprintf(stderr, "INQUIRY failed: req sense %x\n", req_sense);
		exit(1);
	}

	memcpy(vendor_id, data + 8, 8);
	vendor_id[8] = '\0';

	memcpy(product_id, data + 16, 16);
	product_id[16] = '\0';

	fprintf(stdout, "vendor id %s\n", vendor_id);
	fprintf(stdout, "product id %s\n", product_id);

	/* find product id */

	for (i = 0; i < ARRAY_SIZE(id_table); i++) {
		if (strncmp(id_table[i].id, product_id, strlen(id_table[i].id)) == 0)
			break;
	}

	if (i >= ARRAY_SIZE(id_table)) {
		fprintf(stderr, "error: unknown product id\n");
		exit(1);
	}

	/* check h_id */

	if ((h_id != 0) && (h_id != (id_table[i].type & 0x00ffffffffffffffull))) {
		fprintf(stderr, "error: h_id mismatch, expected %llx\n",
			(id_table[i].type & 0x00ffffffffffffffull));
		exit(1);
	}

	/* eject medium */

	if (verbose)
		fprintf(stdout, "=== START STOP (0x1b) ===\n");

	ret = start_stop(device, 0, 1, 0, &req_sense);
	if (ret) {
		fprintf(stderr, "START STOP failed: req sense %x\n", req_sense);
		exit(1);
	}

	/* test unit ready */

	if (verbose)
		fprintf(stdout, "=== TEST UNIT READY (0x00) ===\n");

	ret = test_unit_ready(device, &req_sense);
	if (ret && (req_sense != 0x23a00)) {
		fprintf(stderr, "TEST UNIT READY failed: req sense %x\n", req_sense);
		exit(1);
	}

	/* write firmware in chunks of size MAX_WRITE_BUFFER_LENGTH to buffer BUFFER_ID */

	for (offset = 0; offset < firmware_len; ) {
		nwrite = firmware_len - offset;
		if (nwrite > MAX_WRITE_BUFFER_LENGTH)
			nwrite = MAX_WRITE_BUFFER_LENGTH;

		/* write buffer */

		if (verbose) {
			fprintf(stdout, "=== WRITE BUFFER (0x3b): offset %x length %x ===\n",
				offset, nwrite);
		}

		/* mode 0x7: download microcode with offsets and save */

		ret = write_buffer(device, BUFFER_ID, 0x7, offset, firmware + offset, nwrite, &req_sense);
		if (ret) {
			fprintf(stderr, "WRITE BUFFER failed: req sense %x (%s)\n",
				req_sense, req_sense_message(req_sense));
			exit(1);
		}

		offset += nwrite;
	}

	free(firmware);

	/* poll result */

	timeout = timeout_sec * 1000 * 1000;

	while (timeout > 0) {
		if (verbose)
			fprintf(stdout, "=== TEST UNIT READY (0x00) ===\n");

		ret = test_unit_ready(device, &req_sense);

		fprintf(stdout, "req sense %x (%s)\n", req_sense, req_sense_message(req_sense));

		if (ret && (req_sense == 0x23a00))
			break;

		usleep(100000);

		timeout -= 100000;
	}

	exit(0);
}
