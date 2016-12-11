// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Definition of the DummyAudioSource class.
 * @see audio/audio_source.h
 * @see tests/dummy_audio_source.cpp
 */

#include <cstdint>

#include "../audio/audio.h"
#include "../audio/audio_source.h"
#include "dummy_audio_source.h"

AudioSource::DecodeResult DummyAudioSource::Decode()
{
	auto state = run_out ? AudioSource::DecodeState::END_OF_FILE
	                     : AudioSource::DecodeState::DECODING;
	return std::make_pair(state, AudioSource::DecodeVector());
}

std::uint8_t DummyAudioSource::ChannelCount() const
{
	return 2;
}

std::uint32_t DummyAudioSource::SampleRate() const
{
	return 44100;
}

SampleFormat DummyAudioSource::OutputSampleFormat() const
{
	return SampleFormat::PACKED_SIGNED_INT_32;
}

std::uint64_t DummyAudioSource::Seek(std::uint64_t position)
{
	this->position = position;
	return this->position;
}

const std::string &DummyAudioSource::Path() const
{
	return this->path;
}

std::uint64_t DummyAudioSource::Length() const
{
	return 0;
}
