// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Definition of the DummyAudio_source class.
 * @see audio/audio_source.h
 * @see tests/dummy_audio_source.cpp
 */

#include <cstdint>

#include "../audio/audio_source.h"
#include "../audio/sample_format.h"
#include "dummy_audio_source.h"

Audio_source::Decode_result Dummy_audio_source::Decode()
{
	auto state = run_out ? Audio_source::Decode_state::eof
	                     : Audio_source::Decode_state::decoding;
	return std::make_pair(state, Audio_source::Decode_vector());
}

std::uint8_t Dummy_audio_source::ChannelCount() const
{
	return 2;
}

std::uint32_t Dummy_audio_source::SampleRate() const
{
	return 44100;
}

Sample_format Dummy_audio_source::OutputSampleFormat() const
{
	return Sample_format::sint32;
}

std::uint64_t Dummy_audio_source::Seek(std::uint64_t position)
{
	this->position = position;
	return this->position;
}

const std::string &Dummy_audio_source::Path() const
{
	return this->path;
}

std::uint64_t DummyAudioSource::Length() const
{
	return 0;
}
