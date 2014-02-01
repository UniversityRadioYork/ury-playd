 /*
 * errors.c - error reporting
 *   Part of cuppa, the Common URY Playout Package Architecture
 *
 * Contributors:  Matt Windsor <matt.windsor@ury.org.uk>
 */

/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
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
