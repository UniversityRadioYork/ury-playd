/* 
 * messages.h - does exactly what it says on the tin
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
