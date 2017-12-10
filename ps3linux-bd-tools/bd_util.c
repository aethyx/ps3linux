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
#include <endian.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <scsi/sg.h>
#include <scsi/scsi_ioctl.h>

#include <openssl/aes.h>
#include <openssl/des.h>

#include "bd_util.h"

int
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

void
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

unsigned char *
read_file(const char *path, int *data_len)
{
	FILE *fp;
	unsigned char *data;
	int ret;

	fp = fopen(path, "r");
	if (!fp)
		return (NULL);

	ret = fseek(fp, 0, SEEK_END);
	if (ret) {
		fclose(fp);
		return (NULL);
	}

	*data_len = ftell(fp);

	ret = fseek(fp, 0, SEEK_SET);
	if (ret) {
		fclose(fp);
		return (NULL);
	}

	data = malloc(*data_len);
	if (data == NULL) {
		fclose(fp);
		return (NULL);
	}

	ret = fread(data, 1, *data_len, fp);
	if (ret != *data_len) {
		free(data);
		fclose(fp);
		return (NULL);
	}

	fclose(fp);

	return (data);
}

int
inquiry(const char *device, unsigned char *data, int data_len, unsigned int *req_sense)
{
	int fd;
	struct sg_io_hdr io_hdr;
	unsigned char cmd[256], sense[32];
	int ret;

	fd = open(device, O_RDWR | O_NONBLOCK);
	if (fd < 0)
		return (-1);

	cmd[0] = 0x12;
	cmd[1] = 0x0;
	cmd[2] = 0x0;				/* page code */
	cmd[3] = (data_len >> 8) & 0xff;
	cmd[4] = (data_len & 0xff);
	cmd[5] = 0x0;

	memset(&io_hdr, 0, sizeof(io_hdr));
	io_hdr.interface_id = 'S';
	io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
	io_hdr.timeout = 20000;
	io_hdr.cmdp = cmd;
	io_hdr.cmd_len = 6;
	io_hdr.dxferp = data;
	io_hdr.dxfer_len = data_len;
	io_hdr.sbp = sense;
	io_hdr.mx_sb_len = sizeof(sense);

	ret = ioctl(fd, SG_IO, &io_hdr);
	if (ret) {
		close(fd);
		return (-1);
	}

	close(fd);

	if (io_hdr.status) {
		*req_sense = (sense[2] << 16) | (sense[12] << 8) | sense[13];
		return (-2);
	}

	return (0);
}

int
start_stop(const char *device, unsigned immed, unsigned int loej, unsigned int start,
    unsigned int *req_sense)
{
	int fd;
	struct sg_io_hdr io_hdr;
	unsigned char cmd[256], sense[32];
	int ret;

	fd = open(device, O_RDWR | O_NONBLOCK);
	if (fd < 0)
		return (-1);

	cmd[0] = 0x1b;
	cmd[1] = immed & 0x1;
	cmd[2] = 0x0;
	cmd[3] = 0x0;
	cmd[4] = ((loej & 0x1) << 1) | (start & 0x1);
	cmd[5] = 0x0;

	memset(&io_hdr, 0, sizeof(io_hdr));
	io_hdr.interface_id = 'S';
	io_hdr.dxfer_direction = SG_DXFER_TO_DEV;
	io_hdr.timeout = 20000;
	io_hdr.cmdp = cmd;
	io_hdr.cmd_len = 6;
	io_hdr.dxferp = NULL;
	io_hdr.dxfer_len = 0;
	io_hdr.sbp = sense;
	io_hdr.mx_sb_len = sizeof(sense);

	ret = ioctl(fd, SG_IO, &io_hdr);
	if (ret) {
		close(fd);
		return (-1);
	}

	close(fd);

	if (io_hdr.status) {
		*req_sense = (sense[2] << 16) | (sense[12] << 8) | sense[13];
		return (-2);
	}

	return (0);
}

int
test_unit_ready(const char *device, unsigned int *req_sense)
{
	int fd;
	struct sg_io_hdr io_hdr;
	unsigned char cmd[256], sense[32];
	int ret;

	fd = open(device, O_RDWR | O_NONBLOCK);
	if (fd < 0)
		return (-1);

	memset(cmd, 0, 6);

	memset(&io_hdr, 0, sizeof(io_hdr));
	io_hdr.interface_id = 'S';
	io_hdr.dxfer_direction = SG_DXFER_TO_DEV;
	io_hdr.timeout = 20000;
	io_hdr.cmdp = cmd;
	io_hdr.cmd_len = 6;
	io_hdr.dxferp = NULL;
	io_hdr.dxfer_len = 0;
	io_hdr.sbp = sense;
	io_hdr.mx_sb_len = sizeof(sense);

	ret = ioctl(fd, SG_IO, &io_hdr);
	if (ret) {
		close(fd);
		return (-1);
	}

	close(fd);

	if (io_hdr.status) {
		*req_sense = (sense[2] << 16) | (sense[12] << 8) | sense[13];
		return (-2);
	}

	return (0);
}

int
write_buffer(const char *device, int buffer, unsigned char mode, unsigned int offset,
    unsigned char *data, int data_len, unsigned int *req_sense)
{
	int fd;
	struct sg_io_hdr io_hdr;
	unsigned char cmd[256], sense[32];
	int ret;

	fd = open(device, O_RDWR | O_NONBLOCK);
	if (fd < 0)
		return (-1);

	cmd[0] = 0x3b;
	cmd[1] = mode;
	cmd[2] = buffer;
	cmd[3] = (offset >> 16) & 0xff;
	cmd[4] = (offset >> 8) & 0xff;
	cmd[5] = offset & 0xff;
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
		close(fd);
		return (-1);
	}

	close(fd);

	if (io_hdr.status) {
		*req_sense = (sense[2] << 16) | (sense[12] << 8) | sense[13];
		return (-2);
	}

	return (0);
}

int
send_key(const char *device, unsigned char key_class, unsigned short param_list_length,
    unsigned char agid, unsigned char key_fmt, unsigned char *param_list,
    unsigned int *req_sense)
{
	int fd;
	struct sg_io_hdr io_hdr;
	unsigned char cmd[256], sense[32];
	int ret;

	fd = open(device, O_RDWR | O_NONBLOCK);
	if (fd < 0)
		return (-1);

	cmd[0] = 0xa3;
	cmd[1] = 0x00;
	cmd[2] = 0x00;
	cmd[3] = 0x00;
	cmd[4] = 0x00;
	cmd[5] = 0x00;
	cmd[6] = 0x00;
	cmd[7] = key_class;
	*(uint16_t *) (cmd + 8) = htobe16(param_list_length);
	cmd[10] = ((agid & 0x3) << 6) | (key_fmt & 0x3f);
	cmd[11] = 0x00;

	memset(&io_hdr, 0, sizeof(io_hdr));
	io_hdr.interface_id = 'S';
	io_hdr.dxfer_direction = SG_DXFER_TO_DEV;
	io_hdr.timeout = 20000;
	io_hdr.cmdp = cmd;
	io_hdr.cmd_len = 12;
	io_hdr.dxferp = param_list;
	io_hdr.dxfer_len = param_list_length;
	io_hdr.sbp = sense;
	io_hdr.mx_sb_len = sizeof(sense);

	ret = ioctl(fd, SG_IO, &io_hdr);
	if (ret) {
		close(fd);
		return (-1);
	}

	close(fd);

	if (io_hdr.status) {
		*req_sense = (sense[2] << 16) | (sense[12] << 8) | sense[13];
		return (-2);
	}

	return (0);
}

int
report_key(const char *device, unsigned char key_class, unsigned int alloc_length,
    unsigned char agid, unsigned char key_fmt, unsigned char *buf,
    unsigned int *req_sense)
{
	int fd;
	struct sg_io_hdr io_hdr;
	unsigned char cmd[256], sense[32];
	int ret;

	fd = open(device, O_RDWR | O_NONBLOCK);
	if (fd < 0)
		return (-1);

	cmd[0] = 0xa4;
	cmd[1] = 0x00;
	cmd[2] = 0x00;
	cmd[3] = 0x00;
	cmd[4] = 0x00;
	cmd[5] = 0x00;
	cmd[6] = 0x00;
	cmd[7] = key_class;
	*(uint16_t *) (cmd + 8) = htobe16(alloc_length);
	cmd[10] = ((agid & 0x3) << 6) | (key_fmt & 0x3f);
	cmd[11] = 0x00;

	memset(&io_hdr, 0, sizeof(io_hdr));
	io_hdr.interface_id = 'S';
	io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
	io_hdr.timeout = 20000;
	io_hdr.cmdp = cmd;
	io_hdr.cmd_len = 12;
	io_hdr.dxferp = buf;
	io_hdr.dxfer_len = alloc_length;
	io_hdr.sbp = sense;
	io_hdr.mx_sb_len = sizeof(sense);

	ret = ioctl(fd, SG_IO, &io_hdr);
	if (ret) {
		close(fd);
		return (-1);
	}

	close(fd);

	if (io_hdr.status) {
		*req_sense = (sense[2] << 16) | (sense[12] << 8) | sense[13];
		return (-2);
	}

	return (0);
}

int
send_e1(const char *device, unsigned char param_list_length, const unsigned char cdb[8],
    unsigned char *param_list, unsigned int *req_sense)
{
	int fd;
	struct sg_io_hdr io_hdr;
	unsigned char cmd[256], sense[32];
	int ret;

	fd = open(device, O_RDWR | O_NONBLOCK);
	if (fd < 0)
		return (-1);

	memset(cmd, 0, 12);
	cmd[0] = 0xe1;
	cmd[1] = 0x00;
	cmd[2] = param_list_length;
	cmd[3] = 0x00;
	memcpy(cmd + 4, cdb, 8);

	memset(&io_hdr, 0, sizeof(io_hdr));
	io_hdr.interface_id = 'S';
	io_hdr.dxfer_direction = SG_DXFER_TO_DEV;
	io_hdr.timeout = 20000;
	io_hdr.cmdp = cmd;
	io_hdr.cmd_len = 12;
	io_hdr.dxferp = param_list;
	io_hdr.dxfer_len = param_list_length;
	io_hdr.sbp = sense;
	io_hdr.mx_sb_len = sizeof(sense);

	ret = ioctl(fd, SG_IO, &io_hdr);
	if (ret) {
		close(fd);
		return (-1);
	}

	close(fd);

	if (io_hdr.status) {
		*req_sense = (sense[2] << 16) | (sense[12] << 8) | sense[13];
		return (-2);
	}

	return (0);
}

int
send_e0(const char *device, unsigned int alloc_length, const unsigned char cdb[8],
    unsigned char *buf, unsigned int *req_sense)
{
	int fd;
	struct sg_io_hdr io_hdr;
	unsigned char cmd[256], sense[32];
	int ret;

	fd = open(device, O_RDWR | O_NONBLOCK);
	if (fd < 0)
		return (-1);

	cmd[0] = 0xe0;
	cmd[1] = 0x00;
	cmd[2] = alloc_length;
	cmd[3] = 0x00;
	memcpy(cmd + 4, cdb, 8);

	memset(&io_hdr, 0, sizeof(io_hdr));
	io_hdr.interface_id = 'S';
	io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
	io_hdr.timeout = 20000;
	io_hdr.cmdp = cmd;
	io_hdr.cmd_len = 12;
	io_hdr.dxferp = buf;
	io_hdr.dxfer_len = alloc_length;
	io_hdr.sbp = sense;
	io_hdr.mx_sb_len = sizeof(sense);

	ret = ioctl(fd, SG_IO, &io_hdr);
	if (ret) {
		close(fd);
		return (-1);
	}

	close(fd);

	if (io_hdr.status) {
		*req_sense = (sense[2] << 16) | (sense[12] << 8) | sense[13];
		return (-2);
	}

	return (0);
}

int
sacd_d7_set(const char *device, unsigned char flag, unsigned int *req_sense)
{
	int fd;
	struct sg_io_hdr io_hdr;
	unsigned char cmd[256], data[256], sense[32];
	int ret;

	fd = open(device, O_RDWR | O_NONBLOCK);
	if (fd < 0)
		return (-1);

	cmd[0] = 0xd7;
	cmd[1] = 0x1a;
	cmd[2] = 0x0e;
	cmd[3] = 0x0f;
	cmd[4] = 0x00;
	cmd[5] = 0x00;
	cmd[6] = 0x06;
	cmd[7] = 0x72;
	cmd[8] = 0x00;
	cmd[9] = 0x00;
	cmd[10] = 0x00;
	cmd[11] = flag;

	memset(&io_hdr, 0, sizeof(io_hdr));
	io_hdr.interface_id = 'S';
	io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
	io_hdr.timeout = 20000;
	io_hdr.cmdp = cmd;
	io_hdr.cmd_len = 12;
	io_hdr.dxferp = data;
	io_hdr.dxfer_len = 0x72;
	io_hdr.sbp = sense;
	io_hdr.mx_sb_len = sizeof(sense);

	ret = ioctl(fd, SG_IO, &io_hdr);
	if (ret) {
		close(fd);
		return (-1);
	}

	close(fd);

	if (io_hdr.status) {
		*req_sense = (sense[2] << 16) | (sense[12] << 8) | sense[13];
		return (-2);
	}

	return (0);
}

int
sacd_d7_get(const char *device, unsigned char *flag, unsigned int *req_sense)
{
	int fd;
	struct sg_io_hdr io_hdr;
	unsigned char cmd[256], data[256], sense[32];
	int ret;

	fd = open(device, O_RDWR | O_NONBLOCK);
	if (fd < 0)
		return (-1);

	cmd[0] = 0xd7;
        cmd[1] = 0x1a;
        cmd[2] = 0x0f;
        cmd[3] = 0x0f;
        cmd[4] = 0x00;
        cmd[5] = 0x00;
        cmd[6] = 0x06;
        cmd[7] = 0x72;
        cmd[8] = 0x00;
        cmd[9] = 0x00;
        cmd[10] = 0x00;
        cmd[11] = 0x00;

	memset(&io_hdr, 0, sizeof(io_hdr));
	io_hdr.interface_id = 'S';
	io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
	io_hdr.timeout = 20000;
	io_hdr.cmdp = cmd;
	io_hdr.cmd_len = 12;
	io_hdr.dxferp = data;
	io_hdr.dxfer_len = 0x72;
	io_hdr.sbp = sense;
	io_hdr.mx_sb_len = sizeof(sense);

	ret = ioctl(fd, SG_IO, &io_hdr);
	if (ret) {
		close(fd);
		return (-1);
	}

	close(fd);

	if (io_hdr.status) {
		*req_sense = (sense[2] << 16) | (sense[12] << 8) | sense[13];
		return (-2);
	}

	*flag = data[11];

	return (0);
}

void
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

void
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

void
triple_des_cbc_encrypt(const unsigned char *iv, const unsigned char *key,
    const unsigned char *data, int data_length, unsigned char *out)
{
	DES_cblock kcb1;
	DES_cblock kcb2;
	DES_cblock ivcb;
	DES_key_schedule ks1;
	DES_key_schedule ks2;

	memcpy(kcb1, key, DES_KEY_SZ);
	memcpy(kcb2, key + DES_KEY_SZ, DES_KEY_SZ);
	memcpy(ivcb, iv, DES_KEY_SZ);

	DES_set_key_unchecked(&kcb1, &ks1);
	DES_set_key_unchecked(&kcb2, &ks2);

	DES_ede2_cbc_encrypt(data, out, data_length, &ks1, &ks2, &ivcb, DES_ENCRYPT);
}

void
triple_des_cbc_decrypt(const unsigned char *iv, const unsigned char *key,
    const unsigned char *data, int data_length, unsigned char *out)
{
	DES_cblock kcb1;
	DES_cblock kcb2;
	DES_cblock ivcb;
	DES_key_schedule ks1;
	DES_key_schedule ks2;

	memcpy(kcb1, key, DES_KEY_SZ);
	memcpy(kcb2, key + DES_KEY_SZ, DES_KEY_SZ);
	memcpy(ivcb, iv, DES_KEY_SZ);

	DES_set_key_unchecked(&kcb1, &ks1);
	DES_set_key_unchecked(&kcb2, &ks2);

	DES_ede2_cbc_encrypt(data, out, data_length, &ks1, &ks2, &ivcb, DES_DECRYPT);
}
