/* 
 * constants.h - playout system-wide constants
 *   Part of cuppa, the Common URY Playout Package Architecture
 *
 * Contributors:  Matt Windsor <matt.windsor@ury.org.uk>
 */

/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
 */

#ifndef CUPPA_CONSTANTS_H
#define CUPPA_CONSTANTS_H

#include <stdint.h>		/* int64_t */

/* HOUSEKEEPING: Only put things in macros if they have to be constant at
 * compile-time (for example, array sizes).
 *
 * Also, keeping these grouped in ASCIIbetical order by type first and name
 * second (eg by running them through sort) in both .h and .c would be nice.
 */

#define WORD_LEN 5		/* Length of command words in bytes plus \0 */
//extern const uint64_t	USECS_IN_SEC;	/* Number of microseconds in a second */
const uint64_t	USECS_IN_SEC = 1000000;

#endif				/* !CUPPA_CONSTANTS_H */
