// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the Audio_source class.
 * @see audio/audio_source.h
 */

#include "source.h"

#include <cstdint>

#include "sample_format.h"

namespace playd::audio
{
Source::Source(std::string_view path) : path{path}
{
}

size_t Source::BytesPerSample() const
{
	auto sf = static_cast<uint8_t>(this->OutputSampleFormat());
	return sample_format_bps[sf] * this->ChannelCount();
}

std::string_view Source::Path() const
{
	return this->path;
}

Samples Source::SamplesFromMicros(std::chrono::microseconds micros) const
{
	// The sample rate is expressed in terms of samples per second, so we
	// need to convert the position to seconds then multiply by the rate.
	// We do things in a slightly peculiar order to minimise rounding.
	return (micros.count() * this->SampleRate()) / 1000000;
}

std::chrono::microseconds Source::MicrosFromSamples(Samples samples) const
{
	// This is basically SamplesFromMicros but backwards.
	return std::chrono::microseconds{(samples * 1000000) / this->SampleRate()};
}

} // namespace playd::audio
