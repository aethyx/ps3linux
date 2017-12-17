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
/*
 * A console driver for test OSs.
 *
 */

#include <test.h>

static uval hc_chan;
sval
hcall_put_term_string(uval channel, uval count, const char *str)
{
       uval i;
       union {
               uval64 oct[1];
               uval32 quad[2];
               char c[8];
       } ch;

       for (i = 0; i < count; i++) {
               uval ret;
               uval m = i % sizeof (ch);

               ch.c[m] = str[i];
               if (m == sizeof (ch) - 1 || i == count - 1) {
                       for (;;) {
#ifdef HAS_64BIT
                               ret = hcall_put_term_char(NULL, channel, m + 1,
                                                         ch.oct[0],
                                                         ch.oct[1]);
#else
                               ret = hcall_put_term_char(NULL, channel, m + 1,
                                                         ch.quad[0],
                                                         ch.quad[1],
                                                         ch.quad[2],
                                                         ch.quad[3]);
#endif
                               if (ret != H_Busy) {
                                       break;
                               }
                               yield(1);
                       }
                       if (ret != H_Success) {
                               return ret;
                       }
               }
       }
       return H_Success;
}

sval
hcall_get_term_string(int channel, char str[16])
{
       uval data[4];
       sval ret;

       do {
               ret = hcall_get_term_char(data, channel);

               if (ret == H_Success && data[0] > 0) {
                       ret = (sval)data[0];
                       memcpy(str, &data[1], ret);
                       return ret;
               }
       } while (ret < 0);
       return ret;
}


static sval
hcall_cons_write(struct io_chan *ops __attribute__ ((unused)),
		 const char *buf, uval length)
{
	hcall_put_term_string(hc_chan, length, buf);
	return length;
}

static sval
hcall_cons_write_avail(struct io_chan *ops __attribute__ ((unused)))
{
	return 1;
}

static sval
hcall_cons_read(struct io_chan *ops __attribute__ ((unused)),
		char *buf, uval max_length)
{
	max_length = max_length;

	const char msg[] = "hcall_cons_read() not implemented!\n";
	memcpy(buf, msg, sizeof (msg));
	return sizeof (msg);
}

static sval
hcall_cons_read_avail(struct io_chan *ops __attribute__ ((unused)))
{
	return 1;
}

/* you probably want to bprintInit(&hcall_cons, 0); */
static struct io_chan hcall_cons = {
	.ic_write = hcall_cons_write,
	.ic_write_avail = hcall_cons_write_avail,
	.ic_read = hcall_cons_read,
	.ic_read_avail = hcall_cons_read_avail,
};

extern uval
hcall_cons_init(uval chan)
{
	hc_chan = chan;
	hout_set(&hcall_cons); /* setup hprintf for printing */
	return 1;
}
