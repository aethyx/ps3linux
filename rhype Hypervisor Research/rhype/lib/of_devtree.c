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

/* WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 * This code is intended to be used but relocatable routines So PLEASE
 * do not place any global data here including const intergrals or
 * literals.
 * Assert() is ok for string literal usage.. but thats it.
 *
 */


#include <lib.h>
#include <openfirmware.h>
#include <of_devtree.h>

/*
 * We treat memory like an array of uval64.  For the sake of
 * compactness we assume that a short is big enough as an index.
 */
struct ofd_node_s {
	ofdn_t on_ima;
	ofdn_t on_parent;
	ofdn_t on_child;
	ofdn_t on_peer;
	ofdn_t on_io;
	ofdn_t on_next;		/* for search lists */
	ofdn_t on_prev;
	ofdn_t on_prop;
	uval32 on_pathlen;
	uval32 on_last;
	char   on_path[0];

};

struct ofd_prop_s {
	ofdn_t op_ima;
	ofdn_t op_next;
	uval32 op_objsz;
	uval32 op_namesz;
	/* must have 64bit alignenment */
	char   op_data[0]  __attribute__ ((aligned(8)));
};

struct ofd_io_s {
	ofdn_t	oi_ima;
	ofdn_t	oi_node;
	uval64	oi_open   __attribute__ ((aligned(8)));
};

struct ofd_free_s {
	ofdn_t	of_cells;
	ofdn_t  of_next;
};

struct ofd_mem_s {
	ofdn_t om_num;
	ofdn_t om_next;
	ofdn_t om_free;		/* Future site of a free list */
	ofdn_t _om_pad;
	uval64 om_mem[0] __attribute__((aligned(8)));
};

#define NODE_PAT	0x0f01
#define PROP_PAT	0x0f03
#define IO_PAT		0x0f05


uval
ofd_size(uval mem)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	uval sz;

	sz = m->om_next * sizeof (uval64) + sizeof(*m);
	return sz;
}

uval
ofd_space(uval mem)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	uval sz;

	sz = m->om_num * sizeof (uval64);
	return sz;
}


static uval
ofd_pathsplit_right(const char *s, int c, uval max)
{
	uval i = 0;
	if (max == 0) {
		--max;
	}
	while (*s != '\0' && *s != c && max != 0) {
		++i;
		++s;
		--max;
	}
	return (i);
}

static uval
ofd_pathsplit_left(const char *p, int c, uval len)
{
	const char *s;

	if (len > 0) {
		/* move s to the end */
		s = p + len - 1;

		/* len could include a null */
		if (*s == '\0') {
			--s;
		}
		while (s >= p) {
			if (*s == c) {
				++s;
				break;
			}
			--s;
		}
		if (s < p) {
			return 0;
		}
		return (uval)(s - p);
	}
	return 0;
}

void *
ofd_create(void *mem, uval sz)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	struct ofd_node_s *n;
	uval sum;
	ofdn_t cells;

	if (sz < (sizeof (*n) * 4)) {
		return NULL;
	}

	memset(mem, 0, sz);

	m->om_num = (sz - sizeof(*m))/ sizeof (uval64);

	/* skip the first cell */
	m->om_next = OFD_ROOT;
	n = (struct ofd_node_s *)&m->om_mem[m->om_next];
	n->on_ima = NODE_PAT;
	n->on_pathlen = 2;
	n->on_last = 1;
	n->on_path[0] = '/';
	n->on_path[1] = '\0';

	sum = sizeof (*n) + 2;	/* Don't forget the path */
	cells = (sum + sizeof (m->om_mem[0]) - 1) / sizeof (m->om_mem[0]);
	m->om_next += cells;

	return m;
}

static struct ofd_node_s *
ofd_node_get(struct ofd_mem_s *m, ofdn_t n)
{
	if (n < m->om_next) {
		struct ofd_node_s *r;

		r = (struct ofd_node_s *)&m->om_mem[n];
		if (r->on_ima == NODE_PAT) {
			return r;
		}
		assert(r->on_ima == NODE_PAT, "bad object\n");
	}
	return NULL;
}

ofdn_t
ofd_node_parent(void *mem, ofdn_t n)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	struct ofd_node_s *r = ofd_node_get(m, n);
	return r->on_parent;
}

ofdn_t
ofd_node_peer(void *mem, ofdn_t n)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	struct ofd_node_s *r;

	if (n == 0) {
		return OFD_ROOT;
	}

	r = ofd_node_get(m, n);
	return r->on_peer;
}

const char *
ofd_node_path(void *mem, ofdn_t n)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	struct ofd_node_s *r = ofd_node_get(m, n);
	return r->on_path;
}

static ofdn_t
ofd_node_prop(void *mem, ofdn_t n)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	struct ofd_node_s *r = ofd_node_get(m, n);
	return r->on_prop;
}

ofdn_t
ofd_node_child(void *mem, ofdn_t p)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	struct ofd_node_s *r = ofd_node_get(m, p);
	return r->on_child;
}

sval
ofd_node_to_path(void *mem, ofdn_t p, void *buf, uval sz)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	struct ofd_node_s *r = ofd_node_get(m, p);

	if (sz > r->on_pathlen) {
		sz = r->on_pathlen;
	}

	memcpy(buf, r->on_path, sz);

	return r->on_pathlen;
}

static int
ofd_check(void *p, uval l)
{
	uval i;
	uval64 *v = (uval64 *)p;
	for (i = 0; i < l; i++) {
		if (v[i] != 0ULL) {
			return 0;
		}
	}
	return 1;
}



static ofdn_t
ofd_node_create(struct ofd_mem_s *m, const char *path, uval pathlen)
{
	struct ofd_node_s *n;
	ofdn_t pos;
	uval sum = pathlen + 1 + sizeof (*n); //add trailing zero to path
	ofdn_t cells = (sum + sizeof(m->om_mem[0]) - 1) / sizeof(m->om_mem[0]);

	if (m->om_next + cells >= m->om_num) {
		return 0;
	}

	pos = m->om_next;

	assert(ofd_check(&m->om_mem[pos], cells), "non-zero");
	m->om_next += cells;

	n = (struct ofd_node_s *)&m->om_mem[pos];
	assert(n->on_ima == 0, "new node not empty");

	n->on_ima = NODE_PAT;
	n->on_peer = 0;
	n->on_child = 0;
	n->on_io = 0;
	n->on_pathlen = pathlen;
	n->on_last = ofd_pathsplit_left(path, '/', pathlen);
	strncpy(n->on_path, path, pathlen);
	n->on_path[n->on_pathlen] = 0;
	return pos;
}

/* prunes a node and all its children simply by wasting memory and
 * unlinking it from the tree */
int
ofd_node_prune(void *mem, ofdn_t node)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	struct ofd_node_s *n;
	struct ofd_node_s *p;

	n = ofd_node_get(m, node);
	p = ofd_node_get(m, n->on_parent);
	if (p->on_child == node) {
		/* easy case */
		p->on_child = n->on_peer;
	} else {
		struct ofd_node_s *s;

		s = ofd_node_get(m, p->on_child);
		while (s->on_peer != node) {
			s = ofd_node_get(m, s->on_peer);
		}
		s->on_peer = n->on_peer;
	}
	return 1;
}




ofdn_t
ofd_node_child_create(void *mem, ofdn_t parent, const char *path, uval pathlen)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	struct ofd_node_s *p;
	struct ofd_node_s *n;
	ofdn_t pos;

	p = ofd_node_get(m, parent);
	pos = ofd_node_create(m, path, pathlen);
	n = ofd_node_get(m, pos);

	assert(p->on_child == 0, "child exists\n");
	if (p->on_child == 0) {
		p->on_child = pos;
		n->on_parent = parent;
	} else {
		pos = 0;
	}

	return pos;
}

ofdn_t
ofd_node_peer_create(void *mem, ofdn_t sibling,
		     const char *path, uval pathlen)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	struct ofd_node_s *s;
	struct ofd_node_s *n;
	ofdn_t pos;

	s = ofd_node_get(m, sibling);
	pos = ofd_node_create(m, path, pathlen);
	n = ofd_node_get(m, pos);

	assert(s->on_peer == 0, "peer exists\n");
	if (s->on_peer == 0) {
		s->on_peer = pos;
		n->on_parent = s->on_parent;
	} else {
		pos = 0;
	}
	return pos;
}

static ofdn_t
ofd_node_peer_last(void *mem, ofdn_t c)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	struct ofd_node_s *n;

	n = ofd_node_get(m, c);
	while (n->on_peer > 0) {
		c = n->on_peer;
		n = ofd_node_get(m, c);
	}
	return c;
}

static ofdn_t
ofd_node_walk(struct ofd_mem_s *m, ofdn_t p, const char *s)
{
	struct ofd_node_s *np;
	ofdn_t n;
	ofdn_t r;

	if (*s == '/') {
		++s;
		if (*s == '\0') {
			assert(0, "ends in /\n");
			return 0;
		}
	}

	np = ofd_node_get(m, p);
	r = p;
	do {
		uval last = np->on_last;
		uval lsz = np->on_pathlen - last;
		uval sz;

		sz = ofd_pathsplit_right(s, '/', 0);

		if (lsz > 0 &&
		    strncmp(s, &np->on_path[last], sz) == 0) {
			if (s[sz] == '\0') {
				return r;
			}
			/* there is more to the path */
			n = ofd_node_child(m, p);
			if (n != 0) {
				r = ofd_node_walk(m, n, &s[sz]);
				return r;
			}
			/* there are no children */
			return 0;
		}
	} while (0);
	/* we know that usually we are only serching for top level
	 * peers so we do peers first */
	/* peer */
	n = ofd_node_peer(m, p);
	if (n > 0) {
		r = ofd_node_walk(m, n, s);
	} else {
		r = 0;
	}
	return r;
}


ofdn_t
ofd_node_find(void *mem, const char *devspec)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	ofdn_t n = OFD_ROOT;
	const char *s = devspec;
	uval sz;

	if (s == NULL || s[0] == '\0') {
		return 0;
	}
	if (s[0] != '/') {
		uval asz;

		/* get the component length */
		sz = ofd_pathsplit_right(s, '/', 0);


		/* check for an alias */
		asz = ofd_pathsplit_right(s, ':', sz);

		if (s[asz] == ':') {
			/*
			 * s points to an alias and &s[sz] points to
			 * the alias args.
			 */
			assert(0, "aliases no supported\n");
			return 0;
		}
	} else if (s[1] == '\0') {
		return n;
	}

	n = ofd_node_child(m, n);
	if (n==0) {
		return 0;
	}
	return ofd_node_walk(m, n, s);
}


static struct ofd_prop_s *
ofd_prop_get(struct ofd_mem_s *m, ofdn_t p)
{
	if (p < m->om_next) {
		struct ofd_prop_s *r;

		r = (struct ofd_prop_s *)&m->om_mem[p];
		if (r->op_ima == PROP_PAT) {
			return r;
		}
		assert(r->op_ima == PROP_PAT, "bad object\n");
	}
	return NULL;
}


static ofdn_t
ofd_prop_create(struct ofd_mem_s *m, ofdn_t node, const char *name,
		const void *src, uval sz)
{
	struct ofd_node_s *n;
	struct ofd_prop_s *p;
	uval len = strlen(name) + 1;
	uval sum = sizeof (*p) + sz + len;
	ofdn_t cells;
	char *dst;
	ofdn_t pos;

	cells = (sum + sizeof (m->om_mem[0]) - 1) / sizeof (m->om_mem[0]);

	if (m->om_next + cells >= m->om_num) {
		return 0;
	}
	/* actual data structure */
	pos = m->om_next;
	assert(ofd_check(&m->om_mem[pos], cells), "non-zero");

	p = (struct ofd_prop_s *)&m->om_mem[pos];
	m->om_next += cells;

	assert(p->op_ima == 0, "new node not empty");
	p->op_ima = PROP_PAT;
	p->op_next = 0;
	p->op_objsz = sz;
	p->op_namesz = len;

	/* the rest of the data */
	dst = p->op_data;

	/* zero what will be the pad, cheap and cannot hurt */
	m->om_mem[m->om_next - 1] = 0;

	if (sz > 0) {
		/* some props have no data, just a name */
		memcpy(dst, src, sz);
		dst += sz;
	}

	memcpy(dst, name, len);

	/* now place it in the tree */
	n = ofd_node_get(m, node);
	if (n->on_prop == 0) {
		n->on_prop = pos;
	} else {
		ofdn_t pn = n->on_prop;
		struct ofd_prop_s *nxt;

		for (;;) {
			nxt = ofd_prop_get(m, pn);
			if (nxt->op_next == 0) {
				nxt->op_next = pos;
				break;
			}
			pn = nxt->op_next;
		}
	}
	return pos;
}

void
ofd_prop_remove(void *mem, ofdn_t node, ofdn_t prop)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	struct ofd_node_s *n = ofd_node_get(m, node);
	struct ofd_prop_s *p = ofd_prop_get(m, prop);

	if (n->on_prop == prop) {
		n->on_prop = p->op_next;
	} else {
		ofdn_t pn = n->on_prop;
		struct ofd_prop_s *nxt;

		for (;;) {
			nxt = ofd_prop_get(m, pn);
			if (nxt->op_next == prop) {
				nxt->op_next = p->op_next;
				break;
			}
			pn = nxt->op_next;
		}
	}
	return;
}

ofdn_t
ofd_prop_find(void *mem, ofdn_t n, const char *name)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	ofdn_t p = ofd_node_prop(m, n);
	struct ofd_prop_s *r;
	char *s;
	uval len;

	if (name == NULL || *name == '\0') {
		return 0;
	}

	len = strlen(name) + 1;

	while (p != 0) {
		r = ofd_prop_get(m, p);
		s = &r->op_data[r->op_objsz];
		if (len == r->op_namesz) {
			if (strncmp(name, s, r->op_namesz) == 0) {
				break;
			}
		}
		p = r->op_next;
	}
	return p;
}

static ofdn_t
ofd_prop_next(struct ofd_mem_s *m, ofdn_t n, const char *prev)
{
	ofdn_t p;

	if (prev == NULL || *prev == '\0') {
		/* give the first */
		p = ofd_node_prop(m, n);
	} else {
		struct ofd_prop_s *r;

		/* look for the name */
		p = ofd_prop_find(m, n, prev);
		if (p != 0) {
			/* get the data for prev */
			r = ofd_prop_get(m, p);

			/* now get next */
			p = r->op_next;
		} else {
			p = -1;
		}
	}
	return p;
}

ofdn_t
ofd_nextprop(void *mem, ofdn_t n, const char *prev, char *name)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	ofdn_t p = ofd_prop_next(m, n, prev);
	struct ofd_prop_s *r;
	char *s;

	if (p > 0) {
		r = ofd_prop_get(m, p);
		s = &r->op_data[r->op_objsz];
		memcpy(name, s, r->op_namesz);
	}

	return p;
}

sval
ofd_getcells(void* mem, ofdn_t n, uval32* addr_cells, uval32* size_cells)
{
	if (addr_cells) *addr_cells = 0;
	if (size_cells) *size_cells = 0;
retry:
	if (addr_cells && !*addr_cells) {
		ofd_getprop(mem, n, "#address-cells",
			    addr_cells, sizeof(uval32));
	}

	if (size_cells && !*size_cells) {
		ofd_getprop(mem, n, "#size-cells", size_cells, sizeof(uval32));
	}

	if ((size_cells && !*size_cells) ||
	    (addr_cells && !*addr_cells)) {
		if (n != OFD_ROOT) {
			n = ofd_node_parent(mem, n);
			goto retry;
		}
		return -1;
	}

	return 1;
}

sval
ofd_getprop(void *mem, ofdn_t n, const char *name, void *buf, uval sz)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	ofdn_t p = ofd_prop_find(m, n, name);
	struct ofd_prop_s *r;

	if (p == 0) {
		return -1;
	}

	r = ofd_prop_get(m, p);

	if (sz > r->op_objsz) {
		sz = r->op_objsz;
	}
	memcpy(buf, r->op_data, sz);

	return r->op_objsz;
}

sval
ofd_getproplen(void *mem, ofdn_t n, const char *name)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	ofdn_t p = ofd_prop_find(m, n, name);
	struct ofd_prop_s *r;

	if (p == 0) {
		return -1;
	}

	r = ofd_prop_get(m, p);

	return r->op_objsz;
}

static ofdn_t
ofd_prop_set(void *mem, ofdn_t n, const char *name, const void *src, uval sz)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	ofdn_t p = ofd_prop_find(m, n, name);
	struct ofd_prop_s *r;
	char *dst;

	r = ofd_prop_get(m, p);

	if (sz <= r->op_objsz) {
		/* we can reuse */
		memcpy(r->op_data, src, sz);
		if (sz < r->op_objsz) {
			/* need to move name */
			dst = r->op_data + sz;
			/* use the name arg size we may have overlap
			 * with the original */
			memcpy(dst, name, r->op_namesz);
			r->op_objsz = sz;
		}
	} else {
		/*
		 * Sadly, we remove from the list, wasting the space
		 * and then we can creat a new one
		 */
		ofd_prop_remove(m, n, p);
		p = ofd_prop_create(mem, n, name, src, sz);
	}
	return p;
}

sval
ofd_setprop(void *mem, ofdn_t n, const char *name,
	    const void *buf, uval sz)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	ofdn_t r;

	r = ofd_prop_find(m, n, name);
	if (r == 0) {
		r = ofd_prop_create(mem, n, name, buf, sz);
	} else {
		r = ofd_prop_set(mem, n, name, buf, sz);
	}

	if (r > 0) {
		struct ofd_prop_s *pp = ofd_prop_get(m, r);
		return pp->op_objsz;
	}
	return OF_FAILURE;
}


static ofdn_t
ofd_find_by_prop(struct ofd_mem_s *m, ofdn_t head, ofdn_t prev, ofdn_t n,
		 const char *name, const void *val, uval sz)
{
	struct ofd_node_s *np;
	struct ofd_prop_s *pp;
	ofdn_t p;

retry:
	p = ofd_prop_find(m, n, name);

	if (p > 0) {
		uval match = 0;

		/* a property exists by that name */
		if (val == NULL) {
			match = 1;

		} else {
			/* need to compare values */
			pp = ofd_prop_get(m, p);
			if (sz == pp->op_objsz &&
			    memcmp(pp->op_data, val, sz) == 0) {
				match = 1;
			}
		}
		if (match == 1) {
			if (prev >= 0) {
				np = ofd_node_get(m, prev);
				np->on_next = n;
			} else {
				head = n;
			}
			np = ofd_node_get(m, n);
			np->on_prev = prev;
			np->on_next = -1;

			prev = n;
		}
	}
	p = ofd_node_child(m, n);
	if (p > 0) {
		head = ofd_find_by_prop(m, head, prev, p, name, val, sz);
	}
	p = ofd_node_peer(m, n);
	if (p > 0) {
		// Avoid unnecessary recursion
		// head = ofd_find_by_prop(m, head, prev, p, name, val, sz);
		n = p;
		goto retry;
	}

	return head;
}

ofdn_t
ofd_node_find_by_prop(void *mem, ofdn_t n, const char *name,
		      const void *val, uval sz)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;

	if (n <= 0) {
		n = OFD_ROOT;
	}

	return ofd_find_by_prop(m, -1, -1, n, name, val, sz);
}

ofdn_t
ofd_node_find_next(void *mem, ofdn_t n)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	struct ofd_node_s *np;

	np = ofd_node_get(m, n);
	return np->on_next;
}

ofdn_t
ofd_node_find_prev(void *mem, ofdn_t n)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	struct ofd_node_s *np;

	np = ofd_node_get(m, n);
	return np->on_prev;
}

ofdn_t
ofd_io_create(void *mem, ofdn_t node, uval64 open)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	struct ofd_node_s *n;
	struct ofd_io_s *i;
	ofdn_t pos;
	ofdn_t cells;

	cells = (sizeof (*i) + sizeof (m->om_mem[0]) - 1) /
		sizeof(m->om_mem[0]);

	n = ofd_node_get(m, node);
	if (n == NULL) {
		return 0;
	}
	if (m->om_next + cells >= m->om_num) {
		return 0;
	}

	pos = m->om_next;
	assert(ofd_check(&m->om_mem[pos], cells), "non-zero");

	m->om_next += cells;

	i = (struct ofd_io_s *)&m->om_mem[pos];
	assert(i->oi_ima == 0, "new node not empty");

	i->oi_ima = IO_PAT;
	i->oi_node = node;
	i->oi_open = open;

	n->on_io = pos;

	return pos;
}

static struct ofd_io_s *
ofd_io_get(struct ofd_mem_s *m, ofdn_t i)
{
	if (i < m->om_next) {
		struct ofd_io_s *r;

		r = (struct ofd_io_s *)&m->om_mem[i];
		if (r->oi_ima == IO_PAT) {
			return r;
		}
		assert(r->oi_ima == IO_PAT, "bad object\n");
	}
	return NULL;
}

ofdn_t
ofd_node_io(void *mem, ofdn_t n)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	struct ofd_node_s *r = ofd_node_get(m, n);
	return r->on_io;
}

uval
ofd_io_open(void *mem, ofdn_t n)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	struct ofd_io_s *r = ofd_io_get(m, n);

	return r->oi_open;
}

void
ofd_io_close(void *mem, ofdn_t n)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	struct ofd_io_s *o = ofd_io_get(m, n);
	struct ofd_node_s *r = ofd_node_get(m, o->oi_node);
	o->oi_open = 0;
	r->on_io = 0;
}

ofdn_t
ofd_node_add(void *m, ofdn_t p, const char *path, uval sz)
{
	ofdn_t n;

	n = ofd_node_child(m, p);
	if (n > 0) {
		n = ofd_node_peer_last(m, n);
		if (n > 0) {
			n = ofd_node_peer_create(m, n, path, sz);
		}
	} else {
		n = ofd_node_child_create(m, p, path, sz);
	}
	return n;
}

ofdn_t
ofd_prop_add(void *mem, ofdn_t n, const char *name,
	     const void *buf, uval sz)
{
	struct ofd_mem_s *m = (struct ofd_mem_s *)mem;
	ofdn_t r;

	r = ofd_prop_find(m, n, name);
	if (r == 0) {
		r = ofd_prop_create(mem, n, name, buf, sz);
	} else {
		r = ofd_prop_set(mem, n, name, buf, sz);
	}
	return r;
}


