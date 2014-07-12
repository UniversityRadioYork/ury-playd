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


#endif // PS_CONSTANTS_H
