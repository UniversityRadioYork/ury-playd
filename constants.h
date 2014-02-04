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

#include <chrono>

#include <stdint.h>		/* int64_t */

/* HOUSEKEEPING: Only put things in macros if they have to be constant at
 * compile-time (for example, array sizes).
 */

#define SPINUP_SIZE (2 * BUFFER_SIZE)	/* Num. bytes to load before playing */

const std::chrono::microseconds POSITION_PERIOD(500000);
const std::chrono::nanoseconds LOOP_PERIOD(1000);

const size_t	BUFFER_SIZE = (size_t)FF_MIN_BUFFER_SIZE;
const size_t	RINGBUF_SIZE = (size_t)(1 << 16);

/* See constants.c for more constants (especially macro-based ones) */


#endif				/* not CONSTANTS_H */
