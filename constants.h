// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Various numerical constant definitions.
 * @see messages.h
 */

#ifndef PS_CONSTANTS_H
#define PS_CONSTANTS_H

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

/// The size of the internal decoding buffer.
const size_t BUFFER_SIZE = (size_t)FF_MIN_BUFFER_SIZE;

/// The number of bytes to pre-load into the buffer before playing.
const size_t SPINUP_SIZE = 2 * BUFFER_SIZE;

/// n, where 2^n is the capacity of the AudioOutput ring buffer.
/// @see RINGBUF_SIZE
const size_t RINGBUF_POWER = (size_t)16;

/// The capacity of the AudioOutput ring buffer.
/// @see RINGBUF_POWER
const size_t RINGBUF_SIZE = (size_t)(1 << 16);

#endif // PS_CONSTANTS_H
