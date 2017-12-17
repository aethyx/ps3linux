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

#ifndef _LIB_H
#define _LIB_H

#ifndef __ASSEMBLY__

#include <config.h>
#include <types.h>
#include <stdarg.h>

/* These need to be defined early, to avoid nasty include dependencies */

extern void assprint(const char *expr, const char *file, int line,
		     const char *fmt, ...)
	__attribute__ ((format(printf, 4, 5)));

#define assert(EX,MSG...)					\
	do {							\
		if (!(EX)) {					\
			assprint(#EX, __FILE__, __LINE__, MSG);	\
		}						\
	} while (0)

#include <io_chan.h>

/* print.c */
extern int sprintf(char *buf, const char *fmt, ...)
	__attribute__ ((format(printf, 2, 3)));
extern int snprintf(char *buf, size_t size, const char *fmt, ...)
	__attribute__ ((format(printf, 3, 4)));
extern int vsprintf(char *buf, const char *fmt, va_list ap);
extern int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);

extern sval hprintf(const char *fmt, ...)
	__attribute__ ((format(printf, 1, 2), no_instrument_function));
extern sval vhprintf(const char *fmt, va_list ap)
	__attribute__ ((no_instrument_function));
extern sval _vhprintf(const char *fmt, va_list ap, int lk)
	__attribute__ ((no_instrument_function));
extern void hputs(const char *buf) __attribute__ ((no_instrument_function));


extern struct io_chan *hout_set(struct io_chan *out);
extern struct io_chan *hout_get(void);

extern const char *hstrerror(sval herr);

/* Miscellaneous one-offs. */

extern void breakpoint(void);
extern void *memcpy(void *dest, const void *src, size_t n);
extern void *memmove(void *dest, const void *src, size_t n);
extern void copydown(void *dest, void *src, uval n);
extern void *memset(void *s, int c, size_t n);
extern char *strncpy(char *d, const char *s, size_t n);
extern int strcmp(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern char *strstr(const char *str, const char *sub);
extern int memcmp(const void *v1, const void *v2, size_t n);
extern size_t strlen(const char *s);
extern size_t strnlen(const char *s, size_t maxlen);
extern unsigned long int strtoul(const char *nptr, char **endptr, int base);
extern sval char2hex(char c);
extern char hex2char(uval x);
extern char str2hex(const char *str);
extern uval str2uval(const char *str, uval bytes);


static __inline__ int
atoi(const char *p)
{
	return (int)strtoul(p, NULL, 10);
}

/* First range is a superset of the second range */
static inline uval
range_subset(const uval start1, const uval size1,
	     const uval start2, const uval size2)
{
	return (start1 <= start2 && start2 + size2 <= start1 + size1);
}

static inline uval
range_contains(const uval start, const uval size, const uval val)
{
	return range_subset(start, size, val, 1);
}

static inline uval
ranges_conflict(const uval start1, const uval size1,
		const uval start2, const uval size2)
{
	if (range_contains(start2, size2, start1))
		return 1;

	if (range_contains(start1, size1, start2))
		return 1;

	return 0;
}

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#endif /* ! __ASSEMBLY__ */

#endif /* ! _LIB_H */
