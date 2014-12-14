// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the PlayerFile class, and associated types.
 * @see player/player_file.hpp
 */

#include <cassert>
#include <cstdint>

#include "../audio/audio_system.hpp"
#include "../io/io_response.hpp"
#include "player_file.hpp"
#include "player_position.hpp"

#ifndef _ASSERT_H
#error assert has gone walkies
#endif

PlayerFile::PlayerFile(const ResponseSink *file_sink,
                       const AudioSystem &audio_system)
    : ResponseSource(file_sink), audio(nullptr), audio_system(audio_system)
{
}

void PlayerFile::Eject()
{
	// Don't bother actually ejecting if there isn't anything to eject.
	if (this->audio == nullptr) return;

	this->audio->Stop();
	this->audio = nullptr;
}

void PlayerFile::Load(const std::string &path)
{
	// We can load over existing audio, but we need to eject it first.
	if (this->audio != nullptr) this->Eject();

	this->audio = decltype(this->audio)(this->audio_system.Load(path));

	// Let clients know the file has changed.
	this->Push();
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

void PlayerFile::Emit(const ResponseSink &sink) const
{
	if (this->audio != nullptr) this->audio->Emit(sink);
}

Audio::State PlayerFile::Update()
{
	if (this->audio == nullptr) return Audio::State::NONE;
	return this->audio->Update();
}

std::uint64_t PlayerFile::Position() const
{
	assert(this->audio != nullptr);
	return this->audio->Position();
}

void PlayerFile::Seek(std::uint64_t position)
{
	assert(this->audio != nullptr);
	this->audio->Seek(position);
}
