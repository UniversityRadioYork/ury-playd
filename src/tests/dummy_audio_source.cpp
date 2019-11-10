// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Definition of the DummyAudio_source class.
 * @see audio/audio_source.h
 * @see tests/dummy_audio_source.cpp
 */

#include "dummy_audio_source.h"

#include <cstdint>

#include "../audio/sample_format.h"
#include "../audio/source.h"

namespace Playd::Tests
{
Audio::Source::DecodeResult DummyAudioSource::Decode()
{
	auto state = run_out ? Audio::Source::DecodeState::END_OF_FILE : Audio::Source::DecodeState::DECODING;
	return std::make_pair(state, Audio::Source::DecodeVector());
}

std::uint8_t DummyAudioSource::ChannelCount() const
{
	return 2;
}

std::uint32_t DummyAudioSource::SampleRate() const
{
	return 44100;
}

Audio::SampleFormat DummyAudioSource::OutputSampleFormat() const
{
	return Audio::SampleFormat::SINT32;
}

std::uint64_t DummyAudioSource::Seek(std::uint64_t new_position)
{
	this->position = new_position;
	return this->position;
}

std::string_view DummyAudioSource::Path() const
{
	return this->path;
}

std::uint64_t DummyAudioSource::Length() const
{
	return 0;
}
} // namespace Playd::Tests
