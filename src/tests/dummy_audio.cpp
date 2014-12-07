// This file is part of playd.
// playd is licensed under the MIT licence: see LICENCE.txt.

/**
 * @file
 * Implementation of DummyAudio and related classes.
 */

#include <cstdint>
#include <string>
#include <vector>

#include "../audio/audio.hpp"
#include "../audio/audio_system.hpp"
#include "../io/io_response.hpp"
#include "dummy_audio.hpp"

//
// DummyAudio
//

DummyAudio::DummyAudio(DummyAudioSystem &sys) : sys(sys)
{
}

void DummyAudio::Emit(const ResponseSink &sink) const
{
	sink.Respond(ResponseCode::FILE, this->sys.path);
}

void DummyAudio::Start()
{
	this->sys.started = true;
}

void DummyAudio::Stop()
{
	this->sys.started = false;
}

Audio::State DummyAudio::Update()
{
	return this->sys.state;
}

std::uint64_t DummyAudio::Position() const
{
	return this->sys.pos;
}

void DummyAudio::Seek(std::uint64_t position)
{
	this->sys.pos = position;
}

//
// DummyAudioSystem
//

DummyAudioSystem::DummyAudioSystem() : path(""), pos(0), state(Audio::State::STOPPED)
{
}

Audio *DummyAudioSystem::Load(const std::string &path) const
{
	// Kids, don't try this at home.
	// Were this not a test mock, I'd shoot myself for this!  ~ Matt
	DummyAudioSystem &notconst = const_cast<DummyAudioSystem &>(*this);
	notconst.path = path;
	return new DummyAudio(notconst);
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
