/*******************************************************************************
 * io.h - input/output
 *   Part of cuppa, the Common URY Playout Package Architecture
 *
 * Contributors:  Matt Windsor <matt.windsor@ury.org.uk>
 */

/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
 */

#ifndef CUPPA_IO_H
#define CUPPA_IO_H

#include <stdarg.h>		/* vresponse */

/* Four-character response codes.
 *
 * NOTE: If you're adding new responses here, PLEASE update RESPONSES in io.c.
 */
enum response {
	/* 'Pull' responses (initiated by client command) */
	R_OKAY,			/* Request was valid and produced an answer */
	R_WHAT,			/* Request was invalid/user error */
	R_FAIL,			/* Error, pointing blame at environment */
	R_OOPS,			/* Error, pointing blame at programmer */
	R_NOPE,			/* Request was probably valid, but forbidden. */
	/* 'Push' responses (initiated by server) */
	R_OHAI,			/* Server starting up */
	R_TTFN,			/* Server shutting down */
	R_STAT,			/* Server changing state */
	R_TIME,			/* Server sending current song time */
	R_DBUG,			/* Debug information */
	/* Queue-specific responses */
	R_QENT,			/* Requested information about a Queue ENTry */
	R_QMOD,			/* A command caused a Queue MODification */
	R_QPOS,			/* The current Queue POSition has changed */
	R_QNUM,			/* Reminder of current number of queue items */
	/*--------------------------------------------------------------------*/
	NUM_RESPONSES		/* Number of items in enum */
};

enum response	response(enum response code, const char *format,...);
enum response	vresponse(enum response code, const char *format, va_list ap);
int		input_waiting(void);

#endif				/* !CUPPA_IO_H */
