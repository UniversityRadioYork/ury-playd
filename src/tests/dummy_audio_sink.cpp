// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Definition of the DummyAudioSink class.
 * @see audio/audio_sink.hpp
 * @see tests/dummy_audio_sink.cpp
 */

#include <cstdint>

#include "../audio/audio.hpp"
#include "../audio/audio_sink.hpp"
#include "dummy_audio_sink.hpp"

void DummyAudioSink::Start()
{
	this->state = Audio::State::PLAYING;
}

void DummyAudioSink::Stop()
{
	this->state = Audio::State::STOPPED;
}

Audio::State DummyAudioSink::State()
{
	return this->state;
}

std::uint64_t DummyAudioSink::Position()
{
	return this->position;
}

void DummyAudioSink::SetPosition(std::uint64_t samples)
{
	this->position = samples;
}

void DummyAudioSink::SourceOut()
{
	this->state = Audio::State::AT_END;
}

void DummyAudioSink::Transfer(AudioSink::TransferIterator &begin, const AudioSink::TransferIterator &end)
{
	begin = end;
}
