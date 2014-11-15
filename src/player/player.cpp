// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file player/player.cpp
 * Main implementation file for the Player class.
 * @see player/player.hpp
 * @see player/player_position.cpp
 * @see player/player_state.cpp
 */

#include <cassert>
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
#include "../time_parser.hpp"
#include "player_state.hpp"
#include "player.hpp"

const std::vector<std::string> Player::FEATURES{ "End", "FileLoad", "PlayStop",
	                                         "Seek", "TimeReport" };

Player::Player(const AudioSystem &audio_system, const TimeParser &time_parser)
    : file(audio_system),
      position(),
      state(),
      time_parser(time_parser),
      end_sink(nullptr)
{
}

bool Player::Update()
{
	if (this->CurrentStateIn(PlayerState::AUDIO_PLAYING_STATES)) {
		this->PlaybackUpdate();
	}
	if (this->CurrentStateIn(PlayerState::AUDIO_LOADED_STATES)) {
		this->file.Update();
	}
	return this->IsRunning();
}

void Player::PlaybackUpdate()
{
	if (this->file.IsStopped()) {
		this->End();
	}
	else {
		this->UpdatePosition();
	}
}

void Player::WelcomeClient(ResponseSink &client) const
{
	client.Respond(ResponseCode::OHAI, MSG_OHAI);
	client.RespondArgs(ResponseCode::FEATURES, FEATURES);
	this->file.Emit(client);
	this->position.Emit(client);
	this->state.Emit(client);
}

void Player::SetResponseSink(ResponseSink &sink)
{
	this->file.SetResponseSink(sink);
	this->position.SetResponseSink(sink);
	this->state.SetResponseSink(sink);
	this->end_sink = &sink;
}

void Player::End()
{
	if (this->end_sink != nullptr) {
		this->end_sink->RespondArgs(ResponseCode::END, {});
	}
	this->Stop();

	// Rewind the file back to the start.  We can't use Seek() here
	// in case End() is called from Seek(); a seek failure could start an
	// infinite loop.
	this->file.SeekToPosition(0);
	this->ResetPosition();
	this->UpdatePosition();
}

//
// Commands
//

CommandResult Player::Eject()
{
	if (!this->CurrentStateIn(PlayerState::AUDIO_LOADED_STATES)) {
		return CommandResult::Invalid(MSG_CMD_NEEDS_LOADED);
	}

	this->file.Eject();
	this->SetState(PlayerState::State::EJECTED);

	return CommandResult::Success();
}

CommandResult Player::Load(const std::string &path)
{
	if (path.empty()) return CommandResult::Invalid(MSG_LOAD_EMPTY_PATH);

	try {
		this->file.Load(path);
		this->ResetPosition();
		this->SetState(PlayerState::State::STOPPED);
	}
	catch (FileError &e) {
		// File errors aren't fatal, so catch them here.
		this->Eject();
		return CommandResult::Failure(e.Message());
	}
	catch (Error &) {
		// Ensure a load failure doesn't leave a corrupted track
		// loaded.
		this->Eject();
		throw;
	}

	return CommandResult::Success();
}

CommandResult Player::Play()
{
	if (!this->CurrentStateIn({ PlayerState::State::STOPPED })) {
		return CommandResult::Invalid(MSG_CMD_NEEDS_STOPPED);
	}

	this->file.Start();
	this->SetState(PlayerState::State::PLAYING);

	return CommandResult::Success();
}

CommandResult Player::Quit()
{
	this->Eject();
	this->SetState(PlayerState::State::QUITTING);

	// Quitting is always a valid command.
	return CommandResult::Success();
}

CommandResult Player::Seek(const std::string &time_str)
{
	if (!this->CurrentStateIn(PlayerState::AUDIO_LOADED_STATES)) {
		return CommandResult::Invalid(MSG_CMD_NEEDS_LOADED);
	}

	TimeParser::MicrosecondPosition position(0);

	try {
		position = this->time_parser.Parse(time_str);
	}
	catch (std::out_of_range) {
		return CommandResult::Invalid(MSG_SEEK_INVALID_UNIT);
	}
	catch (SeekError) {
		return CommandResult::Invalid(MSG_SEEK_INVALID_VALUE);
	}

	try {
		this->file.SeekToPosition(position);
	}
	catch (SeekError) {
		Debug() << "Seek failure" << std::endl;

		// Make it look to the client as if the seek ran off the end of
		// the file.
		this->End();
		return CommandResult::Success();
	}

	// Clean out the position tracker, as the position has abruptly changed.
	this->ResetPosition();
	this->UpdatePosition();

	return CommandResult::Success();
}

CommandResult Player::Stop()
{
	if (!this->CurrentStateIn(PlayerState::AUDIO_PLAYING_STATES)) {
		return CommandResult::Invalid(MSG_CMD_NEEDS_PLAYING);
	}

	this->file.Stop();
	this->SetState(PlayerState::State::STOPPED);

	return CommandResult::Success();
}
