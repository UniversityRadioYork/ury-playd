 /*
 * errors.c - error reporting
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

#ifndef CUPPA_ERRORS_H
#define CUPPA_ERRORS_H

/* Common error macros. */
#define SAFE_CALLOC(err, ptr, count, size) do {			\
	if (*err == E_OK) {					\
		ptr = calloc((size_t)count, size);		\
		if (ptr == NULL)				\
			*err = error(E_NO_MEM,			\
		 	   "couldn't alloc ptr");		\
	}							\
} while (0)
#define ERR_IF_NULL(err, ptr) do {				\
	if (*err == E_OK && ptr == NULL)			\
		*err = error(E_INTERNAL_ERROR,			\
		    "passed NULL, expecting ptr");		\
} while (0)

/* Categories of error.
 *
 * NOTE: If you're adding new errors here, PLEASE update the arrays in errors.c
 * to add a name and blame factor to each new error.
 */
enum error {
	E_OK = 0,		/* No error */
	/* User errors */
	E_NO_FILE,		/* Tried to read nonexistent file */
	E_BAD_STATE,		/* State transition not allowed */
	E_BAD_COMMAND,		/* Command was malformed */
	E_COMMAND_REJECTED,	/* Command was valid but refused */
	E_COMMAND_IGNORED,	/* Command was silently ignored */
	/* Environment errors */
	E_BAD_FILE,		/* Tried to read corrupt file */
	E_BAD_CONFIG,		/* Program improperly configured */
	/* System errors */
	E_AUDIO_INIT_FAIL,	/* Couldn't open audio backend */
	E_INTERNAL_ERROR,	/* General system error, usually fatal */
	E_NO_MEM,		/* Allocation of memory failed */
	/* Misc */
	E_EOF,			/* Reached end of file while reading */
	E_INCOMPLETE,		/* Incomplete computation, try again */
	E_UNKNOWN,		/* Unknown error */
	/*--------------------------------------------------------------------*/
	NUM_ERRORS		/* Number of items in enum */
};

/* Categories of blame for errors. */
enum error_blame {
	EB_USER,		/* End-user is at fault */
	EB_POLICY,		/* Request for disallowed action caused error */
	EB_ENVIRONMENT,		/* Environment is at fault */
	EB_PROGRAMMER,		/* Programmer is at fault */
	/*--------------------------------------------------------------------*/
	NUM_ERROR_BLAMES	/* Number of items in enum */
};

/* Categories of severity for errors. */
enum error_severity {
	ES_NORMAL,		/* The program should recover and continue */
	ES_FATAL,		/* The program should halt ASAP */
	ES_ABORT_NOW,		/* The error handler should abort immediately */
	/*--------------------------------------------------------------------*/
	NUM_ERROR_SEVERITIES	/* Number of items in enum */
};

void		dbug      (const char *format,...);
enum error	error(enum error code, const char *format,...);
enum error_severity severity(enum error code);

#endif				/* !CUPPA_ERRORS_H */
