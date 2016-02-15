// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Definition of the DummyAudioSource class.
 * @see audio/audio_source.hpp
 * @see tests/dummy_audio_source.cpp
 */

#include <cstdint>

#include "../audio/audio.hpp"
#include "../audio/audio_source.hpp"
#include "dummy_audio_source.hpp"

/* static */ std::unique_ptr<AudioSource> DummyAudioSource::Build(const std::string &path)
{
	return std::make_unique<DummyAudioSource>(path);
}

AudioSource::DecodeResult DummyAudioSource::Decode()
{
	return std::make_pair(AudioSource::DecodeState::DECODING, AudioSource::DecodeVector());
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
