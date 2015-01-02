// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file player/player.cpp
 * Main implementation file for the Player class.
 * @see player/player.hpp
 * @see player/player_position.cpp
 */

#include <cassert>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../audio/audio_system.hpp"
#include "../audio/audio.hpp"
#include "../cmd_result.hpp"
#include "../errors.hpp"
#include "../io/io_response.hpp"
#include "../messages.h"
#include "player.hpp"

const std::vector<std::string> Player::FEATURES{ "End", "FileLoad", "PlayStop",
	                                         "Seek", "TimeReport" };

Player::Player(const ResponseSink *sink, AudioSystem &audio, PlayerPosition &position)
    : audio(audio), file(audio.Null()), position(position), is_running(true), sink(sink)
{
}

bool Player::Update()
{
	assert(this->file != nullptr);
	auto as = this->file->Update();

	if (as == Audio::State::AT_END) this->End();
	if (as == Audio::State::PLAYING) {
		// Since the audio is currently playing, the position may have
		// advanced since last update.  So we need to update it.
		this->position.Update(this->file->Position());
	}

	return this->is_running;
}

void Player::WelcomeClient(ResponseSink &client) const
{
	client.Respond(Response(Response::Code::OHAI).Arg(MSG_OHAI));

	auto features = Response(Response::Code::FEATURES);
	for (auto &f : FEATURES) features.Arg(f);
	client.Respond(features);

	this->file->Emit({ Response::Code::FILE, Response::Code::STATE }, &client);
	this->position.Emit(client);
}

void Player::End()
{
	this->Stop();

	// Rewind the file back to the start.  We can't use Player::Seek() here
	// in case End() is called from Seek(); a seek failure could start an
	// infinite loop.
	this->SeekRaw(0);

	// Let upstream know that the file ended by itself.
	// This is needed for auto-advancing playlists, etc.
	this->Respond(Response(Response::Code::END));
}

void Player::Respond(const Response &response) const
{
	if (this->sink != nullptr) this->sink->Respond(response);
}

//
// Commands
//

CommandResult Player::Eject()
{
	assert(this->file != nullptr);
	this->file = this->audio.Null();
	this->file->Emit({ Response::Code::STATE }, this->sink);

	return CommandResult::Success();
}

CommandResult Player::Load(const std::string &path)
{
	if (path.empty()) return CommandResult::Invalid(MSG_LOAD_EMPTY_PATH);

	try {
		assert(this->file != nullptr);
		this->file = this->audio.Load(path);
		this->file->Emit({ Response::Code::FILE, Response::Code::STATE }, this->sink);
		assert(this->file != nullptr);

		this->position.Reset();
	} catch (FileError &e) {
		// File errors aren't fatal, so catch them here.
		this->Eject();
		return CommandResult::Failure(e.Message());
	} catch (Error &) {
		// Ensure a load failure doesn't leave a corrupted track
		// loaded.
		this->Eject();
		throw;
	}

	return CommandResult::Success();
}

CommandResult Player::Play()
{
	assert(this->file != nullptr);

	try {
		this->file->Start();
		this->file->Emit({ Response::Code::STATE }, this->sink);
	} catch (NoAudioError &e) {
		return CommandResult::Invalid(e.Message());
	}

	return CommandResult::Success();
}

CommandResult Player::Quit()
{
	this->Eject();
	this->is_running = false;
	return CommandResult::Success();
}

CommandResult Player::Seek(const std::string &time_str)
{
	assert(this->file != nullptr);

	std::uint64_t pos = 0;
	size_t cpos = 0;
	try {
		// In previous versions, this used to parse a unit at the end.
		// This was removed for simplicity--use baps3-cli etc. instead.
		pos = std::stoull(time_str, &cpos);
	} catch (...) {
		// Should catch std::out_of_range and std::invalid_argument.
		// http://www.cplusplus.com/reference/string/stoull/#exceptions
		return CommandResult::Invalid(MSG_SEEK_INVALID_VALUE);
	}

	// cpos will point to the first character in pos that wasn't a number.
	// We don't want any characters here, so bail if the position isn't at
	// the end of the string.
	auto sl = time_str.length();
	if (cpos != sl) return CommandResult::Invalid(MSG_SEEK_INVALID_VALUE);

	try {
		this->SeekRaw(pos);
	} catch (NoAudioError) {
		return CommandResult::Invalid(MSG_CMD_NEEDS_LOADED);
	} catch (SeekError) {
		Debug() << "Seek failure" << std::endl;

		// Make it look to the client as if the seek ran off the end of
		// the file.
		this->End();
	}

	return CommandResult::Success();
}

void Player::SeekRaw(std::uint64_t pos)
{
	assert(this->file != nullptr);
	this->file->Seek(pos);

	// Clean out the position tracker, as the position has abruptly changed.
	this->position.Reset();
	this->position.Update(this->file->Position());
}

CommandResult Player::Stop()
{
	assert(this->file != nullptr);

	try {
		this->file->Stop();
		this->file->Emit({ Response::Code::STATE }, this->sink);
	} catch (NoAudioError &e) {
		return CommandResult::Invalid(e.Message());
	}

	return CommandResult::Success();
}
