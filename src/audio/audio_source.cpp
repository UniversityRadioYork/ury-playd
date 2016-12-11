// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the AudioSource class.
 * @see audio/audio_source.hpp
 */

#include <cstdint>
#include <string>

#include "audio_source.hpp"
#include "sample_formats.hpp"

AudioSource::AudioSource(const std::string &path) : path(path)
{
}

size_t AudioSource::BytesPerSample() const
{
	auto sf = static_cast<uint8_t>(this->OutputSampleFormat());
	return SAMPLE_FORMAT_BPS[sf] * this->ChannelCount();
}

const std::string &AudioSource::Path() const
{
	return this->path;
}

Samples AudioSource::SamplesFromMicros(microseconds micros) const
{
	// The sample rate is expressed in terms of samples per second, so we
	// need to convert the position to seconds then multiply by the rate.
	// We do things in a slightly peculiar order to minimise rounding.
	return (micros.count() * this->SampleRate()) / 1000000;
}

microseconds AudioSource::MicrosFromSamples(Samples samples) const
{
	// This is basically SamplesFromMicros but backwards.
	return microseconds { (samples * 1000000) / this->SampleRate() };
}
