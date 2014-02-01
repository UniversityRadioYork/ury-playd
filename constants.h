/*-
 * Copyright (C) 2012  University Radio York Computing Team
 *
 * This file is a part of playslave.
 *
 * playslave is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * playslave is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * playslave; if not, write to the Free Software Foundation, Inc., 51 Franklin
 * Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

extern "C" {
#ifdef WIN32
#define inline __inline
#endif
#include <libavcodec/avcodec.h>
#ifdef WIN32
#undef inline
#endif
}

#include <stdint.h>		/* int64_t */

/* HOUSEKEEPING: Only put things in macros if they have to be constant at
 * compile-time (for example, array sizes).
 */

#define SPINUP_SIZE (2 * BUFFER_SIZE)	/* Num. bytes to load before playing */

/* All of these are defined in constants.c.
 *
 * HOUSEKEEPING: Keeping these grouped in ASCIIbetical order by type first and
 * name second (eg by running them through sort) in both .h and .c would be nice.
 */

//extern const long	LOOP_NSECS;	/* Number of nanoseconds between main loops */
//extern const size_t	BUFFER_SIZE;	/* Number of bytes in decoding buffer */
//extern const size_t	RINGBUF_SIZE;	/* Number of samples in ring buffer */
//extern const uint64_t	TIME_USECS;	/* Number of microseconds between TIME pulses */

const long	LOOP_NSECS = 1000;
const size_t	BUFFER_SIZE = (size_t)FF_MIN_BUFFER_SIZE;
const size_t	RINGBUF_SIZE = (size_t)(1 << 16);
const uint64_t	TIME_USECS = 1000000;

/* See constants.c for more constants (especially macro-based ones) */


#endif				/* not CONSTANTS_H */
