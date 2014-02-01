/*******************************************************************************
 * io.c - input/output
 *   Part of cuppa, the Common URY Playout Package Architecture
 *
 * Contributors:  Matt Windsor <matt.windsor@ury.org.uk>
 */

/*-
 * Copyright (c) 2012, University Radio York Computing Team
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define _POSIX_C_SOURCE 200809

#include <stdarg.h>		/* print functions */
#include <stdbool.h>		/* booleans */
#include <stdio.h>		/* printf, fprintf */
#include <time.h>		/* select etc. */

#ifdef WIN32
#include <conio.h>
#else
#include <unistd.h>
#include <sys/select.h>		/* select */
#endif

#include "constants.h"		/* WORD_LEN */
#include "io.h"			/* enum response */

/* Structure of information about how to handle a response. */
struct r_data {
	const char	name [WORD_LEN];	/* Symbolic name of response */
	bool		send_to_stdout;	/* Send response to client? */
	bool		send_to_stderr;	/* Send response to error stream? */
};

/* Data for the responses used by cuppa. */
static const struct r_data RESPONSES[NUM_RESPONSES] = {
	/* Name stdout? stderr? */
	/* Pull */
	{"OKAY", true, false},	/* R_OKAY */
	{"WHAT", true, false},	/* R_WHAT */
	{"FAIL", true, true},	/* R_FAIL */
	{"OOPS", true, true},	/* R_OOPS */
        {"NOPE", true, true},	/* R_NOPE */
        /* Push */
	{"OHAI", true, false},	/* R_OHAI */
	{"TTFN", true, false},	/* R_TTFN */
	{"STAT", true, false},	/* R_STAT */
	{"TIME", true, false},	/* R_TIME */
	{"DBUG", false, true},	/* R_DBUG */
        /* Queue specific */
	{"QPOS", true, false}, 	/* R_QPOS */
        {"QENT", true, false},	/* R_QENT */
	{"QMOD", true, false},	/* R_QMOD */
	{"QNUM", true, false}	/* R_QNUM */
};

/* Sends a response to standard out and, for certain responses, standard error.
 * This is the base function for all system responses.
 */
enum response
vresponse(enum response code, const char *format, va_list ap)
{
	va_list		ap2;
	const struct r_data *r;

	r = &(RESPONSES[(int)code]);
	va_copy(ap2, ap);

	if (r->send_to_stdout) {
		printf("%s ", r->name);
		vprintf(format, ap);
		printf("\n");
                fflush(stdout);
	}
	if (r->send_to_stderr) {
		fprintf(stderr, "%s ", r->name);
		vfprintf(stderr, format, ap2);
		fprintf(stderr, "\n");
                fflush(stderr);
	}
	return code;
}

/* Sends a response to standard out and, for certain responses, standard error.
 * This is a wrapper around 'vresponse'.
 */
enum response
response(enum response code, const char *format,...)
{
	va_list		ap;

	/* LINTED lint doesn't seem to like va_start */
	va_start(ap, format);
	vresponse(code, format, ap);
	va_end(ap);

	return code;
}

/* Returns true if input is waiting on standard in. */
int
input_waiting(void)
{
#ifdef WIN32
	return _kbhit();
#else
	fd_set		rfds;
	struct timeval	tv;

	/* Watch stdin (fd 0) to see when it has input. */
	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
	/* Stop checking immediately. */
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	return select(1, &rfds, NULL, NULL, &tv);
#endif
}
