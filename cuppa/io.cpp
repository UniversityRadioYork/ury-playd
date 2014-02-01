/*******************************************************************************
 * io.c - input/output
 *   Part of cuppa, the Common URY Playout Package Architecture
 *
 * Contributors:  Matt Windsor <matt.windsor@ury.org.uk>
 */
/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
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
