// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the PlayerFile class, and associated types.
 * @see player/player_file.hpp
 */

#include "../io/io_response.hpp"
#include "player_file.hpp"

PlayerFile::PlayerFile(const AudioSystem &audio_system)
    : audio(nullptr), audio_system(audio_system)
{
}

void PlayerFile::Emit(ResponseSink &sink) const
{
	if (this->audio != nullptr) {
		sink.Respond(ResponseCode::FILE, this->audio->Path());
	}
}

void PlayerFile::Load(const std::string &path)
{
	if (this->audio != nullptr) {
		Eject();
	}
	this->audio = decltype(this->audio)(this->audio_system.Load(path));
}

void PlayerFile::Eject()
{
	assert(this->audio != nullptr);
	this->audio->Stop();
	this->audio = nullptr;
}

void PlayerFile::Start()
{
	assert(this->audio != nullptr);
	this->audio->Start();
}

void PlayerFile::Stop()
{
	assert(this->audio != nullptr);
	this->audio->Stop();
}

bool PlayerFile::IsStopped()
{
	assert(this->audio != nullptr);
	return this->audio->IsStopped();
}

void PlayerFile::Update()
{
	assert(this->audio != nullptr);
	this->audio->Update();
}
