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
#include <string.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <scsi/sg.h>
#include <scsi/scsi_ioctl.h>

static struct option long_opts[] = {
	{ "device",	no_argument, NULL, 'd' },
	{ "buffer",	no_argument, NULL, 'b' },
	{ "input",	no_argument, NULL, 'i' },
	{ NULL, 0, NULL, 0 }
};

static const char *device = "/dev/sr0";
static int buffer = -1;
static const char *input = NULL;

static int
parse_opts(int argc, char **argv)
{
	int c;
	char *endptr;

	while ((c = getopt_long(argc, argv, "d:b:i:", long_opts, NULL)) != -1) {
		switch (c) {
		case 'd':
			device = optarg;
		break;
		case 'b':
			buffer = strtoul(optarg, &endptr, 0);
			if (*endptr != '\0' || buffer < 0 || buffer > 255) {
				fprintf(stderr, "invalid buffer specified\n");
				return (-1);
			}
		break;
		case 'i':
			input = optarg;
		break;
		default:
			fprintf(stderr, "invalid option specified: %c\n", c);
			return (-1);
		break;
		}
	}

	if (buffer < 0) {
		fprintf(stderr, "no buffer specified\n");
		return (-1);
	}

	if (!input) {
		fprintf(stderr, "no input specified\n");
		return (-1);
	}

	return (0);
}

static int
read_input(const char *path, unsigned char *buf, size_t maxlen)
{
	FILE *fp;
	int len;
	int ret;

	fp = fopen(path, "r");
	if (!fp)
		return (-1);

	ret = fseek(fp, 0, SEEK_END);
	if (ret) {
		fclose(fp);
		return (-1);
	}

	len = ftell(fp);

	if (len > maxlen)
		len = maxlen;

	ret = fseek(fp, 0, SEEK_SET);
	if (ret) {
		fclose(fp);
		return (-1);
	}

	ret = fread(buf, 1, len, fp);
	if (ret != len) {
		fclose(fp);
		return (-1);
	}

	fclose(fp);

	return (len);
}

int
main(int argc, char **argv)
{
	int fd;
	struct sg_io_hdr io_hdr;
	unsigned char cmd[256], sense[32], data[0x8000];
	int data_len;
	int ret;

	ret = parse_opts(argc, argv);
	if (ret)
		exit(255);

	fd = open(device, O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		perror("open device");
		exit(1);
	}

	data_len = read_input(input, data, sizeof(data));
	if (data_len <= 0) {
		fprintf(stderr, "invalid input specified\n");
		exit(1);
	}

	printf("input length %d\n", data_len);

	cmd[0] = 0x3b;
	cmd[1] = 0x05;	/* mode */
	cmd[2] = buffer;
	cmd[3] = 0x00;
	cmd[4] = 0x00;
	cmd[5] = 0x00;
	cmd[6] = (data_len >> 16) & 0xff;
	cmd[7] = (data_len >> 8) & 0xff;
	cmd[8] = data_len & 0xff;
	cmd[9] = 0x00;

	memset(&io_hdr, 0, sizeof(io_hdr));
	io_hdr.interface_id = 'S';
	io_hdr.dxfer_direction = SG_DXFER_TO_DEV;
	io_hdr.timeout = 20000;
	io_hdr.cmdp = cmd;
	io_hdr.cmd_len = 10;
	io_hdr.dxferp = data;
	io_hdr.dxfer_len = data_len;
	io_hdr.sbp = sense;
	io_hdr.mx_sb_len = sizeof(sense);

	ret = ioctl(fd, SG_IO, &io_hdr);
	if (ret) {
		perror("ioctl");
		exit(1);
	}

	if (io_hdr.status) {
		fprintf(stderr, "status %d host status %d driver status %d\n",
		    io_hdr.status, io_hdr.host_status, io_hdr.driver_status);
		exit(1);
	}

	close(fd);

	exit(0);
}
