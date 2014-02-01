/*******************************************************************************
 * io.h - input/output
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
