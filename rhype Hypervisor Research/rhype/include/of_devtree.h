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
#ifndef _OF_DEVTREE_H
#define _OF_DEVTREE_H

#include <types.h>
#include <util.h>

union of_pci_hi {
	uval32 word;
	struct {
		/* *INDENT-OFF* */
		uval32	opa_n: 1; /* relocatable */
		uval32	opa_p: 1; /* prefetchable */
		uval32	opa_t: 1; /* aliased */
		uval32 _opa_res: 3;
		uval32	opa_s: 2; /* space code */
		uval32  opa_b: 8; /* bus number */
		uval32	opa_d: 5; /* device number */
		uval32	opa_f: 3; /* function number */
		uval32	opa_r: 8; /* register number */
		/* *INDENT-ON* */
	} bits;
};

struct of_pci_addr_s {
	union of_pci_hi opa_hi;
	uval32 opa_mid;
	uval32 opa_lo;
};

struct of_pci_range32_s {
	struct of_pci_addr_s opr_addr;
	uval32 opr_phys;
	uval32 opr_size;
};

struct of_pci_range64_s {
	struct of_pci_addr_s opr_addr;
	uval32 opr_phys_hi;
	uval32 opr_phys_lo;
	uval32 opr_size_hi;
	uval32 opr_size_lo;
};

struct of_pci_addr_range64_s {
	struct of_pci_addr_s opr_addr;
	uval32 opr_size_hi;
	uval32 opr_size_lo;
};

struct reg_property32 {
	uval32 address;
	uval32 size;
};

typedef sval32 ofdn_t;

#define OFD_ROOT 1
#define OFD_DUMP_NAMES	0x1
#define OFD_DUMP_VALUES	0x2
#define OFD_DUMP_ALL	(OFD_DUMP_VALUES|OFD_DUMP_NAMES)

extern void *ofd_create(void *mem, uval sz);
extern ofdn_t ofd_node_parent(void *mem, ofdn_t n);
extern ofdn_t ofd_node_peer(void *mem, ofdn_t n);
extern ofdn_t ofd_node_child(void *mem, ofdn_t p);
extern const char *ofd_node_path(void *mem, ofdn_t p);
extern sval ofd_node_to_path(void *mem, ofdn_t p, void *buf, uval sz);
extern ofdn_t ofd_node_child_create(void *mem, ofdn_t parent,
				    const char *path, uval pathlen);
extern ofdn_t ofd_node_peer_create(void *mem, ofdn_t sibling,
				   const char *path, uval pathlen);
extern ofdn_t ofd_node_find(void *mem, const char *devspec);
extern ofdn_t ofd_node_add(void *m, ofdn_t n, const char *path, uval sz);
extern int ofd_node_prune(void *m, ofdn_t n);
extern ofdn_t ofd_node_io(void *mem, ofdn_t n);

extern ofdn_t ofd_nextprop(void *mem, ofdn_t n, const char *prev, char *name);
extern ofdn_t ofd_prop_find(void *mem, ofdn_t n, const char *name);
extern sval ofd_getprop(void *mem, ofdn_t n, const char *name,
			void *buf, uval sz);
extern sval ofd_getproplen(void *mem, ofdn_t n, const char *name);

extern sval ofd_setprop(void *mem, ofdn_t n, const char *name,
			const void *buf, uval sz);
extern void ofd_prop_remove(void *mem, ofdn_t node, ofdn_t prop);
extern ofdn_t ofd_prop_add(void *mem, ofdn_t n, const char *name,
			   const void *buf, uval sz);
extern ofdn_t ofd_io_create(void *m, ofdn_t node, uval64 open);
extern uval ofd_io_open(void *mem, ofdn_t n);
extern void ofd_io_close(void *mem, ofdn_t n);


typedef void (*walk_fn)(void *m, ofdn_t p, uval arg);
extern void ofd_dump_props(void *m, ofdn_t p, uval arg);

extern void ofd_walk(void *m, ofdn_t p, walk_fn fn, uval arg);


/* Recursively look up #address_cells and #size_cells properties */
extern sval ofd_getcells(void *mem, ofdn_t n,
			 uval32 *addr_cells, uval32 *size_cells);

extern uval ofd_size(uval mem);
extern uval ofd_space(uval mem);

extern void ofd_prop_print(const char *head, const char *path,
			   const char *name, const char *prop, uval sz);

extern ofdn_t ofd_node_find_by_prop(void *mem, ofdn_t n, const char *name,
				    const void *val, uval sz);
extern ofdn_t ofd_node_find_next(void *mem, ofdn_t n);
extern ofdn_t ofd_node_find_prev(void *mem, ofdn_t n);

#endif /* _OF_DEVTREE_H */
