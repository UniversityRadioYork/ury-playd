// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the Audio_source class.
 * @see audio/audio_source.h
 */

#include <cstdint>

#include "audio_source.h"
#include "sample_format.h"

Audio_source::Audio_source(const std::string &path) : path(path)
{
}

size_t Audio_source::BytesPerSample() const
{
	auto sf = static_cast<uint8_t>(this->OutputSampleFormat());
	return sample_format_bps[sf] * this->ChannelCount();
}

const std::string &Audio_source::Path() const
{
	return this->path;
}

Samples Audio_source::SamplesFromMicros(microseconds micros) const
{
	// The sample rate is expressed in terms of samples per second, so we
	// need to convert the position to seconds then multiply by the rate.
	// We do things in a slightly peculiar order to minimise rounding.
	return (micros.count() * this->SampleRate()) / 1000000;
}

microseconds Audio_source::MicrosFromSamples(Samples samples) const
{
	// This is basically SamplesFromMicros but backwards.
	return microseconds { (samples * 1000000) / this->SampleRate() };
}
