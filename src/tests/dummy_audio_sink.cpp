// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Definition of the DummyAudio_sink class.
 * @see audio/audio_sink.h
 * @see tests/dummy_audio_sink.cpp
 */

#include "dummy_audio_sink.h"

#include "../audio/sink.h"

namespace Playd::Tests
{
void DummyAudioSink::Start()
{
	this->state = Audio::Sink::State::PLAYING;
}

void DummyAudioSink::Stop()
{
	this->state = Audio::Sink::State::STOPPED;
}

Audio::Sink::State DummyAudioSink::CurrentState()
{
	return this->state;
}

std::uint64_t DummyAudioSink::Position()
{
	return this->position;
}

void DummyAudioSink::SetPosition(uint64_t samples)
{
	this->position = samples;
}

void DummyAudioSink::SourceOut()
{
	this->state = Audio::Sink::State::AT_END;
}

size_t DummyAudioSink::Transfer(const gsl::span<const std::byte> src)
{
	return src.size();
}

} // namespace playd::tests
