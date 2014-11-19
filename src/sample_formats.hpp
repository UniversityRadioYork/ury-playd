// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * The SampleFormat enumeration.
 */

#include <cstdint>

#ifndef PLAYD_SAMPLE_FORMATS_HPP
#define PLAYD_SAMPLE_FORMATS_HPP

/**
 * Sample formats available in playd.
 *
 * This is the intersection of those sample formats available in ffmpeg and
 * PortAudio.  In the future, if and when other input and output libraries are
 * used/supported, this may change.
 *
 * Packed formats are those where each sample contains each audio channel, one
 * packed after the other.  Planar formats use separate runs of samples for each
 * channel (each channel has its own sample plane).  For the most part,
 * playd deals in the former, resampling instances of the latter.
 */
enum class SampleFormat : std::uint8_t {
	PACKED_UNSIGNED_INT_8, ///< Packed 8-bit unsigned integer.
	PACKED_SIGNED_INT_16,  ///< Packed 16-bit signed integer.
	PACKED_SIGNED_INT_32,  ///< Packed 32-bit signed integer.
	PACKED_FLOAT_32        ///< Packed 32-bit floating point.
};

#endif // PLAYD_SAMPLE_FORMATS_HPP
