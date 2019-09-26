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

namespace playd::tests
{
audio::Source::Decode_result Dummy_audio_source::Decode()
{
	auto state = run_out ? audio::Source::Decode_state::eof
	                     : audio::Source::Decode_state::decoding;
	return std::make_pair(state, audio::Source::Decode_vector());
}

std::uint8_t Dummy_audio_source::ChannelCount() const
{
	return 2;
}

std::uint32_t Dummy_audio_source::SampleRate() const
{
	return 44100;
}

audio::Sample_format Dummy_audio_source::OutputSampleFormat() const
{
	return audio::Sample_format::sint32;
}

std::uint64_t Dummy_audio_source::Seek(std::uint64_t new_position)
{
	this->position = new_position;
	return this->position;
}

std::string_view Dummy_audio_source::Path() const
{
	return this->path;
}

std::uint64_t Dummy_audio_source::Length() const
{
	return 0;
}
} // namespace playd::tests
