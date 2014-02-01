/*******************************************************************************
 * utils.c - miscellaneous utility macros and functions
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

#include <ctype.h>		/* isspace */
#include <stdlib.h>		/* NULL */

/* Given a char pointer into a null-terminated string, returns the char pointer
 * marking the next non-whitespace character.  If this character is the null
 * terminator '\0', then the end of the string was reached.
 *
 * Returns the null pointer if a null pointer was provided for 'str'.
 */
char           *
skip_space(char *str)
{
	char           *p;

	/* Note: \0 is NOT a space, so this terminates at string end */
	for (p = str; p != NULL && isspace((int)*p); p++);

	return p;
}

/* As 'skip_space', but with the condition reversed: the function skips over
 * all non-space characters (except '\0').
 */
char           *
skip_nonspace(char *str)
{
	char           *p;

	for (p = str; p != NULL && *p != '\0' && !isspace((int)*p); p++);

	return p;
}

/* As 'skip_space', but sets each traversed character to '\0'. */
char           *
nullify_space(char *str)
{
	char           *p;

	for (p = str; p != NULL && isspace((int)*p); p++)
		*p = '\0';

	return p;
}

/* As 'nullify_space', but acts on trailing space.  Supply with a pointer to
 * the terminator of a string; obviously, this function works in-place.
 */
char		*
nullify_tspace(char *end)
{
	char		*p;
	
	for (p = end; p == '\0'; p--);
	for (p = end; p != NULL && isspace((int)*p); p--)
		*p = '\0';

	return p;
}
