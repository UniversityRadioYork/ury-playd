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

#include <stdarg.h>		/* va_list etc. */
#include <stdio.h>		/* snprintf */
#include <stdlib.h>		/* calloc */

#include "errors.h"		/* enum error, enum error_blame */
#include "io.h"			/* vresponse, enum response */
#include "messages.h"		/* MSG_ERR_NOMEM */

/* Structure of information about how to handle an error. */
struct e_data {
	const char     *name;	/* Symbolic name of this error */
	enum error_blame blame;	/* Who to blame for this error */
	enum error_severity severity;	/* Severity of this error */
};

/* Name and behaviour for every error in cuppa.
 * Names should always just be the constant with leading E_ removed.
 * (More human-friendly information should be in the error string.)
 */
static const struct e_data ERRORS[NUM_ERRORS] = {
	{"OK",
		EB_PROGRAMMER,	/* Should never go to error handler */
		ES_NORMAL
	},
	{"NO_FILE",
		EB_USER,
		ES_NORMAL
	},
	{"BAD_STATE",
		EB_USER,
		ES_NORMAL
	},
	{"BAD_COMMAND",
		EB_USER,
		ES_NORMAL
	},
	{"COMMAND_REJECTED",
		EB_POLICY,
		ES_NORMAL
	},
	{"COMMAND_IGNORED",
		EB_PROGRAMMER,	/* Should be ignored, not handled */
		ES_NORMAL
	},
	{"BAD_FILE",
		EB_ENVIRONMENT,
		ES_NORMAL
	},
	{"BAD_CONFIG",
		EB_ENVIRONMENT,
		ES_FATAL
	},
	{"AUDIO_INIT_FAIL",
		EB_ENVIRONMENT,
		ES_FATAL
	},
	{"INTERNAL_ERROR",
		EB_PROGRAMMER,
		ES_NORMAL
	},
	{"NO_MEM",
		EB_ENVIRONMENT,
		ES_FATAL
	},
	{"EOF",
		EB_PROGRAMMER,	/* Should usually never go to error handler */
		ES_NORMAL
	},
	{"INCOMPLETE",
		EB_PROGRAMMER,	/* Should usually never go to error handler */
		ES_NORMAL
	},
	{"UNKNOWN",
		EB_PROGRAMMER,
		ES_FATAL
	},
};

/* This maps error blame factors to response codes. */
const enum response BLAME_RESPONSE[NUM_ERROR_BLAMES] = {
	R_WHAT,			/* EB_USER */
	R_NOPE,			/* EB_POLICY */
	R_FAIL,			/* EB_ENVIRONMENT */
	R_OOPS,			/* EB_PROGRAMMER */
};

/* Sends a debug message. */
void
dbug(const char *format,...)
{
	va_list		ap;

	/* LINTED lint doesn't seem to like va_start */
	va_start(ap, format);
	vresponse(R_DBUG, format, ap);
	va_end(ap);
}

/* Throws an error message.
 *
 * This does not propagate the error anywhere - the error code will need to be
 * sent up the control chain and handled at the top of the player.  It merely
 * sends a response through stdout and potentially stderr to let the client/logs
 * know something went wrong.
 */
enum error
error(enum error code, const char *format,...)
{
	va_list		ap;	/* Variadic arguments */
	va_list		ap2;	/* Variadic arguments */
	char           *buf;	/* Temporary buffer for rendering error */
	size_t		buflen;	/* Length for creating buffer */
	const struct e_data *e;	/* Data for error code; */

	e = &(ERRORS[(int)code]);
	buf = NULL;

	/* LINTED lint doesn't seem to like va_start */
	va_start(ap, format);
	va_copy(ap2, ap);

	/*
	 * Problem: adding the error name into the format without changing
	 * the format string dynamically (a bad idea).
	 *
	 * Our strategy: render the error into a temporary buffer using magicks,
	 * then send it to response.
	 */

	/*
	 * Try printing the error into a null buffer to get required length
	 * (see http://stackoverflow.com/questions/4899221)
	 */
	buflen = vsnprintf(NULL, 0, format, ap);
	buf = (char *)calloc(buflen + 1, sizeof(char));
	if (buf != NULL)
		vsnprintf(buf, buflen + 1, format, ap2);

	response(BLAME_RESPONSE[e->blame], "%s %s",
		 e->name,
		 buf == NULL ? MSG_ERR_NOMEM : buf);
	va_end(ap);

	if (buf != NULL)
		free(buf);

	return code;
}

enum error_severity
severity(enum error code)
{
	return ERRORS[(int)code].severity;
}
