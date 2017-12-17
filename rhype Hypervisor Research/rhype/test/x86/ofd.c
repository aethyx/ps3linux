/*
 * Copyright (C) 2005 Jimi Xenidis <jimix@watson.ibm.com>, IBM Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id$
 */

#include <lib.h>
#include <ofd.h>
#include <test.h>
#include <mmu.h>
#include <hcall.h>
#include <sim.h>
#include <objalloc.h>
#include <sysipc.h>
#include <x86/os_args.h>

/* the number of vscsi servers on the IO partition */
#define NUM_VSCSIS_ON_PARTITION		6
/* the number of logical lans on any partition */
#define NUM_LLAN_PER_PARTITION		2

/* local datatypes */
struct vio_llan_data {
	uval64 llan_mac;
};

struct vio_crq_data {
	uval crq_type;
};

#define CRQ_TYPE_VSCSI        1
#define CRQ_TYPE_VSCSIS       2
#define CRQ_TYPE_VTPM         3
#define CRQ_TYPE_VTPMS        4

struct vio_data {
	uval vd_dma_window;
	uval vd_uaddr;
	uval vd_type;
	union {
		struct vio_llan_data vd_llan_data;
		struct vio_crq_data vd_crq_data;
	} vd_spec;
	uval32 vd_lpid;
	uval32 vd_xirr;
	uval32 vd_liobn;
};

#define TYPE_CRQ		1
#define TYPE_LLAN		2
#define TYPE_VTY		3

#define MAX_VIO_ENTRIES		40

struct ofd {
	char o_cmdlineargs[256];
	uval o_next_vio_entry;
	struct vio_data o_viodata[MAX_VIO_ENTRIES];
};

static struct ofd myofd;
static struct os_args my_os_args;

extern char default_bootargs[256];

/* local function prototypes */
static uval ofd_write(struct ofd *ofd, char *buf, uval sz, char *buf2,
		      uval sz2);

/* beginning of code */

uval
ofd_devtree_init(uval mem __attribute__ ((unused)), uval *space
		 __attribute__ ((unused)))
{
	*space = 0;
	return (uval)&myofd;
}

static uval
ofd_vdevice_add(struct ofd *o, struct vio_data *vd)
{
	uval rc = 0;

	if (o->o_next_vio_entry < MAX_VIO_ENTRIES) {
		o->o_viodata[o->o_next_vio_entry] = *vd;
		o->o_next_vio_entry++;
		rc = 1;
	}
	return rc;
}

static ofdn_t
ofd_vdevice_vty(struct partition_status *ps)
{
	struct vio_data vd;
	uval server;
	uval client;

	vterm_create(ps, &server, &client);

	vd.vd_uaddr = client;
	vd.vd_xirr = -1;
	vd.vd_liobn = client;
	vd.vd_dma_window = 0;
	vd.vd_type = TYPE_VTY;
	vd.vd_lpid = ps->lpid;

	return ofd_vdevice_add(&myofd, &vd);
}

static ofdn_t
ofd_vdevice_llan(uval lpid)
{
	struct vio_data vd;
	uval64 mac = 0x02ULL << 40;	/* local address tag */
	uval32 intr[2] = { /* source */ 0x0, /* +edge */ 0x0 };
	uval dma_sz = 8 * 1024 * 1024;	/* (((0x80<< 12) >> 3) << 12) */
	uval32 dma[5] = {
		/* liobn */ 0x0,
		/* phys  */ 0x0, 0x0,
		/* size  */ 0x0, 0x0
	};
	uval liobn;

	dma_sz = llan_create(dma_sz, lpid, &liobn);

	dma[0] = liobn;
	dma[4] = dma_sz;
	intr[0] = liobn;
	mac |= liobn;

	vd.vd_type = TYPE_LLAN;
	vd.vd_xirr = intr[0];
	vd.vd_dma_window = dma_sz;
	vd.vd_liobn = liobn;
	vd.vd_uaddr = liobn;
	vd.vd_spec.vd_llan_data.llan_mac = mac;
	vd.vd_lpid = lpid;

	return ofd_vdevice_add(&myofd, &vd);
}

static ofdn_t
ofd_vdevice_crq_vscsis(uval lpid)
{
	struct vio_data vd;

	uval dma_sz = 8 * 1024 * 1024;	/* (((0x80<< 12) >> 3) << 12) */
	uval32 liobn;
	ofdn_t r = 0;
	struct crq_maintenance *cm;

	cm = crq_create(lpid, ROLE_SERVER, dma_sz);
	if (NULL != cm) {
		liobn = cm->cm_liobn_server;
		dma_sz = cm->cm_dma_sz;

		vd.vd_type = TYPE_CRQ;
		vd.vd_xirr = liobn;
		vd.vd_dma_window = dma_sz;
		vd.vd_liobn = liobn;
		vd.vd_uaddr = liobn;
		vd.vd_spec.vd_crq_data.crq_type = CRQ_TYPE_VSCSIS;
		vd.vd_lpid = lpid;

		r = ofd_vdevice_add(&myofd, &vd);
	}
	return r;
}

static ofdn_t
ofd_vdevice_crq_vscsi(uval lpid)
{
	struct vio_data vd;

	uval dma_sz = 8 * 1024 * 1024;	/* (((0x80<< 12) >> 3) << 12) */
	uval32 liobn;
	ofdn_t r = 0;
	struct crq_maintenance *cm;

	cm = crq_create(lpid, ROLE_CLIENT, dma_sz);
	if (NULL != cm) {
		liobn = cm->cm_liobn_client;
		dma_sz = cm->cm_dma_sz;

		vd.vd_type = TYPE_CRQ;
		vd.vd_xirr = liobn;
		vd.vd_dma_window = dma_sz;
		vd.vd_liobn = liobn;
		vd.vd_uaddr = liobn;
		vd.vd_spec.vd_crq_data.crq_type = CRQ_TYPE_VSCSI;
		vd.vd_lpid = lpid;

		r = ofd_vdevice_add(&myofd, &vd);
	}
	return r;
}

static ofdn_t
ofd_vdevice_crq_vtpms(uval lpid)
{
	struct vio_data vd;

	uval dma_sz = 2 * 1024 * 1024;
	uval32 liobn;
	ofdn_t r = 0;
	struct crq_maintenance *cm;

	cm = crq_create(lpid, ROLE_SERVER, dma_sz);
	if (NULL != cm) {
		liobn = cm->cm_liobn_server;
		dma_sz = cm->cm_dma_sz;

		vd.vd_type = TYPE_CRQ;
		vd.vd_xirr = liobn;
		vd.vd_dma_window = dma_sz;
		vd.vd_liobn = liobn;
		vd.vd_uaddr = liobn;
		vd.vd_spec.vd_crq_data.crq_type = CRQ_TYPE_VTPMS;
		vd.vd_lpid = lpid;

		r = ofd_vdevice_add(&myofd, &vd);
	}
	return r;
}

static ofdn_t
ofd_vdevice_crq_vtpm(uval lpid)
{
	struct vio_data vd;

	uval dma_sz = 2 * 1024 * 1024;
	uval32 liobn;
	ofdn_t r = 0;
	struct crq_maintenance *cm;

	cm = crq_create(lpid, ROLE_CLIENT, dma_sz);
	if (NULL != cm) {
		liobn = cm->cm_liobn_client;
		dma_sz = cm->cm_dma_sz;

		vd.vd_type = TYPE_CRQ;
		vd.vd_xirr = liobn;
		vd.vd_dma_window = dma_sz;
		vd.vd_liobn = liobn;
		vd.vd_uaddr = liobn;
		vd.vd_spec.vd_crq_data.crq_type = CRQ_TYPE_VTPM;
		vd.vd_lpid = lpid;

		r = ofd_vdevice_add(&myofd, &vd);
	}
	return r;
}

static uval
ofd_vdevice_crq(uval lpid)
{
	if (CONTROLLER_LPID == lpid || 
	    H_SELF_LPID == lpid) {
		int i = 0;

		/*
		 * This is a server partition
		 * Let's just give it some amount of vSCSI server
		 * units and same amount of TPM server units.
		 */
		while (i < NUM_VSCSIS_ON_PARTITION) {
			ofd_vdevice_crq_vscsis(lpid);
			ofd_vdevice_crq_vtpms(lpid);
			i++;
		}
	} else {
		/* This is a client partition */
		ofd_vdevice_crq_vscsi(lpid);
		ofd_vdevice_crq_vtpm(lpid);
	}
	return 1;
}

static ofdn_t
ofd_vdevice(struct partition_status *ps)
{
	int i = 0;
	ofdn_t r = 0;

	ofd_vdevice_vty(ps);
	ofd_vdevice_crq(ps->lpid);
	while (i < NUM_LLAN_PER_PARTITION) {
		r = ofd_vdevice_llan(ps->lpid);
		i++;
	}
	return r;
}

uval
ofd_lpar_create(struct partition_status *ps, uval new, uval mem)
{
	(void)mem;
	(void)new;

	memset(&my_os_args, 0x0, sizeof (my_os_args));
	myofd.o_next_vio_entry = 0;

	if (iohost_lpid == 0) {
		iohost_lpid = ps->lpid;
	}

	hprintf("Building device entries for partition 0x%lx\n", ps->lpid);
	ofd_vdevice(ps);

	ofd_write(&myofd,
		  my_os_args.commandlineargs,
		  sizeof (my_os_args.commandlineargs),
		  my_os_args.vdevargs, sizeof (my_os_args.vdevargs));

	return (uval)&my_os_args;
}

uval
ofd_size(uval lofd)
{
	if (0 == lofd) {
		return 0;
	} else {
		return sizeof (struct os_args);
	}
}

void
ofd_bootargs(uval ofd, char *buf, uval sz)
{
	struct ofd *lofd = (struct ofd *)ofd;

	memcpy(lofd->o_cmdlineargs,
	       buf, MIN(sz, sizeof (lofd->o_cmdlineargs)));
}

static const char *type_strings[] = {
	"ibmvscsis",
	"ibmvscsi",
	"ibmvtpms",
	"ibmvtpm"
};

static uval
ofd_crq_write(const struct vio_data *vd,
	      char *buffer, uval offset, uval bufsize)
{
	sval index = -1;
	uval n;

	switch (vd->vd_spec.vd_crq_data.crq_type) {
	case CRQ_TYPE_VSCSIS:
		index = 0;
		break;
	case CRQ_TYPE_VSCSI:
		index = 1;
		break;
	case CRQ_TYPE_VTPMS:
		index = 2;
		break;
	case CRQ_TYPE_VTPM:
		index = 3;
		break;

	default:
		return offset;
	}
	n = snprintf(&buffer[offset],
		     bufsize - offset - 1,
		     " %s=0x%lx,0x%x,0x%lx,0x%x",
		     type_strings[index],
		     vd->vd_uaddr,
		     vd->vd_liobn,
		     vd->vd_dma_window,
		     vd->vd_xirr);

	return (offset + n);
}

static uval
ofd_llan_write(const struct vio_data *vd,
	       char *buffer, uval offset, uval bufsize)
{
	uval n;
	uval64 mac = vd->vd_spec.vd_llan_data.llan_mac;

	n = snprintf(&buffer[offset],
		     bufsize - offset - 1,
		     " ibmveth=0x%lx,0x%x,0x%lx,0x%x,"
		     "%02llX:%02llX:%02llX:%02llX:%02llX:%02llX,%d",
		     vd->vd_uaddr,
		     vd->vd_liobn,
		     vd->vd_dma_window,
		     vd->vd_xirr,
		     mac & 0xffUL,
		     (mac >> 8) & 0xffUL, (mac >> 16) & 0xffUL,
		     (mac >> 24) & 0xffUL, (mac >> 32) & 0xffUL,
		     (mac >> 40) & 0xffUL, 6);

	return (offset + n);
}

static uval
ofd_vty_write(const struct vio_data *vd,
	      char *buffer, uval offset, uval bufsize)
{
	uval n;

	n = snprintf(&buffer[offset],
		     bufsize - offset - 1,
		     " ibmvty=0x%lx,0x%x,0x%lx,0x%x",
		     vd->vd_uaddr,
		     vd->vd_liobn,
		     vd->vd_dma_window,
		     vd->vd_xirr);

	return (offset + n);
}

static uval
ofd_write(struct ofd *myofd, char *buf, uval sz, char *vdev_buf, uval vdev_sz)
{
	uval i = 0;
	uval len = strlen(default_bootargs);
	uval vdev_len = 0;

	len = MIN(len, sz);

	if (0 != len) {
		strncpy(buf, default_bootargs, len);
	}

	vdev_len = 0;
	while (i < myofd->o_next_vio_entry) {
		if (vdev_len >= vdev_sz) {
			break;
		}
		switch (myofd->o_viodata[i].vd_type) {
		case TYPE_CRQ:
			vdev_len = ofd_crq_write(&myofd->o_viodata[i],
						 vdev_buf, vdev_len, vdev_sz);
			break;

		case TYPE_LLAN:
			vdev_len = ofd_llan_write(&myofd->o_viodata[i],
						  vdev_buf, vdev_len, vdev_sz);
			break;

		case TYPE_VTY:
			vdev_len = ofd_vty_write(&myofd->o_viodata[i],
						 vdev_buf, vdev_len, vdev_sz);
			break;
		}
		i++;
	}
	return 1;
}
