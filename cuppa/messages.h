/* 
 * messages.h - does exactly what it says on the tin
 *   Part of cuppa, the Common URY Playout Package Architecture
 *
 * Contributors:  Matt Windsor <matt.windsor@ury.org.uk>
 */

/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
 */

#ifndef CUPPA_MESSAGES_H
#define CUPPA_MESSAGES_H

/* All of these are defined in messages.c.
 *
 * HOUSEKEEPING: Keep these and their messages.c counterparts in ASCIIbetical
 * order if possible?
 */

extern const char     *MSG_CMD_ARGN;	/* Nullary command got an argument */
extern const char     *MSG_CMD_ARGU;	/* Unary command got no arguments */
extern const char     *MSG_CMD_HITEND; /* Accidentally reached end of commands list */
extern const char     *MSG_CMD_NOPROP; /* Command type is PROPAGATE but prop is NULL */
extern const char     *MSG_CMD_NOSUCH;	/* No command with the given word */
extern const char     *MSG_CMD_NOWORD;	/* No command word given */
extern const char     *MSG_ERR_NOMEM;	/* For when the error routine runs out of mem */

#endif				/* !CUPPA_MESSAGES_H  */
