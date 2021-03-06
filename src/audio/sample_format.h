// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * The SampleFormat enumeration and related declarations.
 * @see audio/sample_formats.cpp
 */

#ifndef PLAYD_SAMPLE_FORMATS_H
#define PLAYD_SAMPLE_FORMATS_H

#include <array>
#include <cstdint>

namespace Playd::Audio
{
/// Type definition for samples.
using Samples = std::uint64_t;

/**
 * Sample formats available in playd.
 *
 * This is the intersection of those sample formats available in the decoders
 * and SDL.
 *
 * Packed formats are those where each sample contains each audio channel, one
 * packed after the other.  Planar formats use separate runs of samples for each
 * channel (each channel has its own sample plane).  For the most part,
 * playd deals in the former, resampling instances of the latter.
 *
 * REMEMBER: changes here will require changes in lookup tables in both
 * sample_formats.cpp and audio_sink.cpp.
 */
enum class SampleFormat : std::uint8_t {
	UINT8,  ///< Packed 8-bit unsigned integer.
	SINT8,  ///< Packed 8-bit signed integer.
	SINT16, ///< Packed 16-bit signed integer.
	SINT32, ///< Packed 32-bit signed integer.
	FLOAT32 ///< Packed 32-bit floating point.
};

/// Number of sample formats available; should agree with SampleFormat.
constexpr std::size_t SAMPLE_FORMAT_COUNT = 5;

/// Map from SampleFormats to bytes-per-mono-sample.
extern const std::array<std::size_t, SAMPLE_FORMAT_COUNT> sample_format_bps;

} // namespace Playd::Audio

#endif // PLAYD_SAMPLE_FORMATS_H
