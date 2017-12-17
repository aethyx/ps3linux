/*
 * Copyright (C) 2005 Michal Ostrowski <mostrows@watson.ibm.com>, IBM Corp.
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
 *
 * Glues a vterm (vty 0 of a specified partition) to stdin/stdout or
 * TCP/IP port.  Transfers data between the two.
 */

#define _GNU_SOURCE
#include <sys/fcntl.h>

#include <hype.h>
#include <hype_util.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <string.h>

static uval ua = 0;
static uval verbose = 0;
static uval h_connected = 0;
static uval s_connected = 1;
static int match_sig;
static int io_sig;

static void
cleanup(int sig, siginfo_t * si, void *ptr)
{
	(void)si;
	(void)ptr;

	if (verbose) {
		fprintf(stderr, "cleanup on signal %d\n", sig);
	}
	if (ua) {
		oh_hcall_args args;

		args.opcode = H_FREE_VTERM;
		args.args[0] = ua;
		hcall(&args);
	}
	exit(0);
}

struct hvpacket {
	struct hvpacket *next;
	int len;
	uval buf[16 / sizeof(uval)];
};

struct hvpacket *input = NULL;
struct hvpacket *output = NULL;
struct hvpacket **input_tail = &input;
struct hvpacket **output_tail = &output;

static void
msg_sig_fn(int sig, siginfo_t * si, void *ptr)
{
	(void)si;
	(void)ptr;

	oh_hcall_args args;

	while (1) {
		struct hvpacket *hp;

		args.opcode = H_GET_TERM_CHAR;
		args.args[0] = ua;
		hcall(&args);
		if (args.retval != 0 || args.args[0] == 0)
			break;
		h_connected = 1;

		hp = malloc(sizeof (struct hvpacket));
		memset(hp->buf, 0, sizeof(hp->buf));
		hp->next = NULL;
		hp->len = args.args[0];
		memcpy(hp->buf, &args.args[1], hp->len);
		if (verbose) {
			fprintf(stderr, "< %02d %*.16s "
				UVAL_CHOOSE("%08lx %08lx", "%016lx %016lx")
				"\n",
				hp->len, hp->len,
				(char *)hp->buf, hp->buf[0], hp->buf[1]);
		}
		*input_tail = hp;
		input_tail = &hp->next;
	}
	if (args.retval != 0 && h_connected)
		exit(0);
	if (sig != match_sig)
		exit(0);

}

struct sigaction cleanup_sig = {
	.sa_sigaction = cleanup,
	.sa_flags = 0,

};

/* Enable this fd to generate signals */
static int
config_fd(int fd, int sig)
{
	int ret = fcntl(fd, F_SETSIG, sig);

	ASSERT(ret >= 0, "F_SETSIG failed: %d\n", errno);
	ret = fcntl(fd, F_GETSIG);

	long flags = fcntl(fd, F_GETFL);

	fcntl(fd, F_SETOWN, getpid());
	fcntl(fd, F_SETFL, flags | O_NONBLOCK | O_ASYNC);
	return 0;
}

/* if port == 0 use stdin/stdout instead of socket */
static void
vterm_io(int port, uval vterm)
{
	oh_hcall_args args;
	int ret;
	int listen_sock = -1;
	int out_fd = 1;
	int in_fd = 0;

	fcntl(in_fd, F_SETFL, O_RDONLY);

	ua = vterm;

	atexit((void (*)(void))cleanup);

	match_sig = SIGRTMIN;
	io_sig = match_sig + 1;

	/* Set up socket to accept connections on */
	if (port) {
		listen_sock = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in sin = {.sin_family = AF_INET,
			.sin_addr = {
				     .s_addr = INADDR_ANY},
			.sin_port = htons(port),
			.sin_zero = ""
		};

		{
			int tmp = 1;

			if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR,
				       (char *)&tmp, sizeof (tmp)) != 0) {
				perror("setsockopt");
				exit(-1);
			}
		}

		ret = bind(listen_sock, (struct sockaddr *)&sin, sizeof (sin));
		if (ret < 0) {
			perror("Binding to port");
			exit(-1);
		}

		listen(listen_sock, 4);
		s_connected = 0;
	} else {
		config_fd(in_fd, io_sig);
	}

	sigset_t set;

	sigfillset(&set);
	sigdelset(&set, SIGINT);
	sigdelset(&set, SIGTERM);
	sigdelset(&set, SIGHUP);
	sigdelset(&set, SIGKILL);

	sigfillset(&cleanup_sig.sa_mask);
	sigaction(SIGINT, &cleanup_sig, NULL);
	sigaction(SIGTERM, &cleanup_sig, NULL);
	sigaction(SIGHUP, &cleanup_sig, NULL);
	sigaction(SIGKILL, &cleanup_sig, NULL);

	sigprocmask(SIG_SETMASK, &set, NULL);

	ret = sig_xirr_bind(ua, match_sig);
	if (ret < 0) {
		perror("Binding to interrupt");
		exit(-1);
	}

	sigemptyset(&set);
	sigaddset(&set, match_sig);
	sigaddset(&set, io_sig);

	int sock = 0;
	struct hvpacket *hp = NULL;

	do {
		while (!s_connected && listen_sock != -1) {
			sock = accept(listen_sock, NULL, 0);
			if (sock < 0 && errno != EINTR) {
				perror("Accept error");
				exit(-1);
			} else if (sock >= 0) {
				s_connected = 1;
				out_fd = in_fd = sock;
				config_fd(in_fd, io_sig);
			}
		}

		while (output) {
			struct hvpacket *p = output;

			args.opcode = H_PUT_TERM_CHAR;
			args.args[0] = ua;
			args.args[1] = p->len;
			args.args[2] = p->buf[0];
			args.args[3] = p->buf[1];
 			if (4 == sizeof(uval)) {
 				args.args[4] = p->buf[2];
 				args.args[5] = p->buf[3];
 			}
			if (verbose) {
				fprintf(stderr,
					"> %02d %*.16s "
					UVAL_CHOOSE("%08lx %08lx",
						    "%016lx %016lx")
					"\n",
					p->len, p->len,
					(char *)p->buf, p->buf[0], p->buf[1]);
			}
			hcall(&args);

			if (args.retval != 0) {
				printf("hcall error: "
				       UVAL_CHOOSE("%d", "%ld") "\n",
				       args.retval);
				break;
			}
			if (!h_connected && args.retval == 0) {
				h_connected = 1;
			}
			output = p->next;
			free(p);
			if (output == NULL)
				output_tail = &output;
		}

		while (input) {
			struct hvpacket *p = input;

			ret = write(out_fd, &p->buf[0], p->len);
			if (ret <= 0) {
				if (port) {
					s_connected = 0;
					close(out_fd);
				}
				break;
			}
			input = p->next;
			free(p);
			if (input == NULL)
				input_tail = &input;
		}

		/* Use signals to figure out whether we should read
		 * from socket or vterm */

		struct timespec t = { 1, 0 };
		int sig = sigtimedwait(&set, NULL, &t);

		ret = 1;
		while (sig == io_sig && ret > 0) {
			char buf[16];

			ret = read(in_fd, buf, sizeof(buf));
			if (ret > 0) {
				hp = malloc(sizeof (struct hvpacket));
				memset(hp->buf, 0, sizeof(hp->buf));
				hp->len = ret;
				memcpy(&hp->buf[0], buf, ret);
				hp->next = NULL;
				*output_tail = hp;
				output_tail = &hp->next;
			}
		}

		if (sig == match_sig) {
			msg_sig_fn(match_sig, NULL, NULL);
		}

	} while (1);

}

static void
usage()
{
	printf("hype_term [-v|--verbose] [-p|--port <port>] "
	       "[-n|--name <lpar>] [-u|--unit <ua>]\n");
}

int
main(int argc, char **argv)
{
	const struct option long_options[] = {
		{"unit", 1, 0, 'u'},
		{"name", 1, 0, 'n'},
		{"port", 1, 0, 'p'},
		{"help", 0, 0, 'h'},
		{"verbose", 1, 0, 'v'}
	};
	short port = 0;
	const char *pname = NULL;

	while (1) {
		int ret = 0;
		int c = getopt_long(argc, argv, "u:n:hvp:",
				    long_options, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'u':
			ua = strtoul(optarg, NULL, 0);
			if (errno == ERANGE)
				ret = -1;
			break;
		case 'n':
			pname = optarg;
			break;
		case 'p':
			port = strtoul(optarg, NULL, 0);
			if (errno == ERANGE)
				ret = -1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'h':
			usage();
			exit(0);

		}
		if (ret < 0) {
			printf("Argument error\n");
			exit(-1);
		}
	}

	printf("ua: %lx\n", ua);
	hcall_init();

	if (ua == 0) {
		oh_set_pname(pname);
		if (get_file_numeric("res_console_srv", &ua) < 0) {
			oh_hcall_args hargs;

			hargs.opcode = H_VIO_CTL;
			hargs.args[0] = HVIO_ACQUIRE;
			hargs.args[1] = HVIO_VTERM;
			hargs.args[2] = PGSIZE;

			int ret = hcall(&hargs);

			ASSERT(ret >= 0 && hargs.retval == 0,
			       "hcall failure: %d "
			       UVAL_CHOOSE("0x%x", "0x%lx") "\n",
			       ret, hargs.retval);
			ua = hargs.args[0];
			set_file_printf("res_console_srv", "0x%llx", ua);
		}
	}

	if (verbose) fprintf(stderr, "Using vterm 0x%lx\n", ua);

	vterm_io(port, ua);
	return 0;
}
