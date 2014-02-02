/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
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
const uint64_t	USECS_IN_SEC = 1000000;

const long	LOOP_NSECS = 1000;
const size_t	BUFFER_SIZE = (size_t)FF_MIN_BUFFER_SIZE;
const size_t	RINGBUF_SIZE = (size_t)(1 << 16);
const uint64_t	TIME_USECS = 1000000;

/* See constants.c for more constants (especially macro-based ones) */


#endif				/* not CONSTANTS_H */
