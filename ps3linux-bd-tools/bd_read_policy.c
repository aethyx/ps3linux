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
	{ NULL, 0, NULL, 0 }
};

static const char *device = "/dev/sr0";

static int
parse_opts(int argc, char **argv)
{
	int c;

	while ((c = getopt_long(argc, argv, "d:", long_opts, NULL)) != -1) {
		switch (c) {
		case 'd':
			device = optarg;
		break;
		default:
			fprintf(stderr, "invalid option specified: %c\n", c);
			return (-1);
		break;
		}
	}

	return (0);
}

int
main(int argc, char **argv)
{
	int fd;
	struct sg_io_hdr io_hdr;
	unsigned char cmd[256], sense[32], data[8];
	int ret;

	ret = parse_opts(argc, argv);
	if (ret)
		exit(255);

	fd = open(device, O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		perror("open device");
		exit(1);
	}

	cmd[0] = 0xa4;	/* REPORT KEY */
	cmd[1] = 0x00;
	cmd[2] = 0x00;
	cmd[3] = 0x00;
	cmd[4] = 0x00;
	cmd[5] = 0x00;
	cmd[6] = 0x00;
	cmd[7] = 0xe0;	/* key class */
	cmd[8] = 0x00;
	cmd[9] = 0x08;	/* allocation length */
	cmd[10] = 0x03;	/* AGID and key format */
	cmd[11] = 0x00; /* control */

	memset(data, 0, sizeof(data));

	memset(&io_hdr, 0, sizeof(io_hdr));
	io_hdr.interface_id = 'S';
	io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
	io_hdr.timeout = 20000;
	io_hdr.cmdp = cmd;
	io_hdr.cmd_len = 12;
	io_hdr.dxferp = data;
	io_hdr.dxfer_len = sizeof(data);
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

	fprintf(stdout, "%02x\n", data[7]);

	close(fd);

	exit(0);
}
