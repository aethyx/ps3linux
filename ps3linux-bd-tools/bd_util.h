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

#ifndef _BD_UTIL_H
#define _BD_UTIL_H

#define ARRAY_SIZE(a)	(sizeof(a) / sizeof(a[0]))

int parse_hex(const char *s, unsigned char *b, int maxlen);

void hex_fprintf(FILE *fp, const unsigned char *buf, size_t len);

unsigned char *read_file(const char *path, int *data_len);

int inquiry(const char *device, unsigned char *data, int data_len, unsigned int *req_sense);

int start_stop(const char *device, unsigned immed, unsigned int loej, unsigned int start,
    unsigned int *req_sense);

int test_unit_ready(const char *device, unsigned int *req_sense);

int write_buffer(const char *device, int buffer, unsigned char mode, unsigned int offset,
    unsigned char *data, int data_len, unsigned int *req_sense);

int send_key(const char *device, unsigned char key_class, unsigned short param_list_length,
    unsigned char agid, unsigned char key_fmt, unsigned char *param_list,
    unsigned int *req_sense);

int report_key(const char *device, unsigned char key_class, unsigned int alloc_length,
    unsigned char agid, unsigned char key_fmt, unsigned char *buf,
    unsigned int *req_sense);

int send_e1(const char *device, unsigned char param_list_length, const unsigned char cdb[8],
    unsigned char *param_list, unsigned int *req_sense);

int send_e0(const char *device, unsigned int alloc_length, const unsigned char cdb[8],
    unsigned char *buf, unsigned int *req_sense);

int sacd_d7_set(const char *device, unsigned char flag, unsigned int *req_sense);

int sacd_d7_get(const char *device, unsigned char *flag, unsigned int *req_sense);

void aes_cbc_encrypt(const unsigned char *iv, const unsigned char *key, int key_length,
    const unsigned char *data, int data_length, unsigned char *out);

void aes_cbc_decrypt(const unsigned char *iv, const unsigned char *key, int key_length,
    const unsigned char *data, int data_length, unsigned char *out);

void triple_des_cbc_encrypt(const unsigned char *iv, const unsigned char *key,
    const unsigned char *data, int data_length, unsigned char *out);

void triple_des_cbc_decrypt(const unsigned char *iv, const unsigned char *key,
    const unsigned char *data, int data_length, unsigned char *out);

#endif
