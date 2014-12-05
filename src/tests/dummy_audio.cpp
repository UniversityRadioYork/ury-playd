// This file is part of playd.
// playd is licensed under the MIT licence: see LICENCE.txt.

/**
 * @file
 * Implementation of DummyAudio and related classes.
 */

#include <string>
#include <vector>

#include "../audio/audio.hpp"
#include "../audio/audio_system.hpp"
#include "../io/io_response.hpp"
#include "../time_parser.hpp"
#include "dummy_audio.hpp"

//
// DummyAudio
//

DummyAudio::DummyAudio() : path(""), pos(0), state(Audio::State::STOPPED)
{
}

void DummyAudio::Emit(const ResponseSink &sink) const
{
	sink.Respond(ResponseCode::FILE, this->path);
}

void DummyAudio::Start()
{
	this->started = true;
}

void DummyAudio::Stop()
{
	this->started = false;
}

Audio::State DummyAudio::Update()
{
	return this->state;
}

TimeParser::MicrosecondPosition DummyAudio::Position() const
{
	return this->pos;
}

void DummyAudio::Seek(TimeParser::MicrosecondPosition position)
{
	this->pos = position;
}

//
// DummyAudioSystem
//

Audio *DummyAudioSystem::Load(const std::string &path) const
{
	this->audio->path = path;
	return this->audio;
}

void DummyAudioSystem::SetDeviceID(int)
{
	// Deliberately ignore
}

std::vector<AudioSystem::Device> DummyAudioSystem::GetDevicesInfo()
{
	return std::vector<AudioSystem::Device>();
}

bool DummyAudioSystem::IsOutputDevice(int)
{
	return false;
}
