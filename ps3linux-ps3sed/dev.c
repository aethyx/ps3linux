/*-
 * Copyright (C) 2011, 2012 glevand <geoffrey.levand@mail.ru>
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

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <asm/ps3strgmngr.h>

#include "dev.h"

static int fd = -1;

int
dev_open(const char *device)
{
	if (fd >= 0)
		close(fd);

	fd = open(device, O_RDWR);
	if (fd < 0)
		return (-1);

	return (0);
}

void
dev_close(void)
{
	if (fd >= 0)
		close(fd);
}

int
dev_get_devices(struct ps3strgmngr_device *dev, int max_devices)
{
	struct ps3strgmngr_ctl_get_devices get_devices;
	int ret;

	if (fd < 0)
		return (-1);

	ret = ioctl(fd, PS3STRGMNGR_CTL_GET_DEVICES, &get_devices);
	if (ret)
		return (ret);

	if (max_devices > get_devices.nr_devices)
		max_devices = get_devices.nr_devices;

	memcpy(dev, get_devices.device, max_devices * sizeof(*dev));

	return (max_devices);
}

int
dev_create_region(uint64_t device_id, uint64_t start_block,
    uint64_t nr_blocks, uint64_t laid, uint64_t *region_id)
{
	struct ps3strgmngr_ctl_create_region create_region;
	int ret;

	if (fd < 0)
		return (-1);

	create_region.device_id = device_id;
	create_region.start_block = start_block;
	create_region.nr_blocks = nr_blocks;
	create_region.laid = laid;

	ret = ioctl(fd, PS3STRGMNGR_CTL_CREATE_REGION, &create_region);
	if (ret)
		return (ret);

	*region_id = create_region.region_id;

	return (0);
}

int
dev_delete_region(uint64_t device_id, uint64_t region_id)
{
	struct ps3strgmngr_ctl_delete_region delete_region;
	int ret;

	if (fd < 0)
		return (-1);

	delete_region.device_id = device_id;
	delete_region.region_id = region_id;

	ret = ioctl(fd, PS3STRGMNGR_CTL_DELETE_REGION, &delete_region);
	if (ret)
		return (ret);

	return (0);
}

int
dev_set_region_acl_entry(uint64_t device_id, uint64_t region_id,
    uint64_t laid, uint64_t access_rights)
{
	struct ps3strgmngr_ctl_set_region_acl_entry set_region_acl_entry;
	int ret;

	if (fd < 0)
		return (-1);

	set_region_acl_entry.device_id = device_id;
	set_region_acl_entry.region_id = region_id;
	set_region_acl_entry.laid = laid;
	set_region_acl_entry.access_rights = access_rights;

	ret = ioctl(fd, PS3STRGMNGR_CTL_SET_REGION_ACL_ENTRY,
	    &set_region_acl_entry);
	if (ret)
		return (ret);

	return (0);
}
