// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Definition of the DummyAudio_sink class.
 * @see audio/audio_sink.h
 * @see tests/dummy_audio_sink.cpp
 */

#include "../audio/audio_sink.h"
#include "dummy_audio_sink.h"

void Dummy_audio_sink::Start()
{
	this->state = Audio_sink::State::playing;
}

void Dummy_audio_sink::Stop()
{
	this->state = Audio_sink::State::stopped;
}

Audio_sink::State Dummy_audio_sink::CurrentState()
{
	return this->state;
}

std::uint64_t Dummy_audio_sink::Position()
{
	return this->position;
}

void Dummy_audio_sink::SetPosition(uint64_t samples)
{
	this->position = samples;
}

void Dummy_audio_sink::SourceOut()
{
	this->state = Audio_sink::State::at_end;
}

size_t Dummy_audio_sink::Transfer(const gsl::span<const uint8_t> src)
{
	return src.length();
}
