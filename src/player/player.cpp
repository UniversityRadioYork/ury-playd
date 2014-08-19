// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

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

#include "../audio/audio_system.hpp"
#include "../audio/audio.hpp"
#include "../errors.hpp"
#include "../io/io_response.hpp"
#include "../messages.h"
#include "player_state.hpp"
#include "player.hpp"

Player::Player(const AudioSystem &audio_system, const Player::TP &time_parser)
    : file(audio_system),
      position(),
      state(),
      time_parser(time_parser),
      end_sink(nullptr)
{
}

bool Player::Update()
{
	if (CurrentStateIn(PlayerState::AUDIO_PLAYING_STATES)) {
		PlaybackUpdate();
	}
	if (CurrentStateIn(PlayerState::AUDIO_LOADED_STATES)) {
		this->file.Update();
	}
	return IsRunning();
}

void Player::PlaybackUpdate()
{
	if (this->file.IsStopped()) {
		if (this->end_sink != nullptr) {
			this->end_sink->Respond(ResponseCode::END, "");
		}
		Stop();
		Seek("0");
	} else {
		UpdatePosition();
	}
}

void Player::WelcomeClient(ResponseSink &client) const
{
	client.Respond(ResponseCode::OHAI, MSG_OHAI);
	client.Respond(ResponseCode::FEATURES, MSG_FEATURES);
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

//
// Commands
//

bool Player::Eject()
{
	if (CurrentStateIn(PlayerState::AUDIO_LOADED_STATES)) {
		this->file.Eject();
		SetState(PlayerState::State::EJECTED);
		return true;
	}
	return false;
}

bool Player::Load(const std::string &path)
{
	bool valid = !path.empty();
	if (valid) {
		try
		{
			this->file.Load(path);
			ResetPosition();
			Debug() << "Loaded " << path << std::endl;
			SetState(PlayerState::State::STOPPED);
		}
		catch (FileError &)
		{
			// File errors aren't fatal, so catch them here.
			Eject();
			valid = false;
		}
		catch (Error &)
		{
			// Ensure a load failure doesn't leave a corrupted track
			// loaded.
			Eject();
			throw;
		}
	}
	return valid;
}

bool Player::Play()
{
	if (CurrentStateIn({ PlayerState::State::STOPPED })) {
		this->file.Start();
		SetState(PlayerState::State::PLAYING);
		return true;
	}
	return false;
}

bool Player::Quit()
{
	Eject();
	SetState(PlayerState::State::QUITTING);

	return true; // Always a valid command.
}

bool Player::Seek(const std::string &time_str)
{
	if (CurrentStateIn(PlayerState::AUDIO_LOADED_STATES)) {
		bool success = true;
		std::chrono::microseconds position(0);

		try
		{
			position = this->time_parser.Parse(time_str);
		}
		catch (std::out_of_range)
		{
			success = false;
		}

		if (success) {
			this->file.SeekToPosition(position);
			this->ResetPosition();
			this->UpdatePosition();
		}
		return success;
	}
	return false;
}

bool Player::Stop()
{
	if (CurrentStateIn(PlayerState::AUDIO_PLAYING_STATES)) {
		this->file.Stop();
		SetState(PlayerState::State::STOPPED);
		return true;
	}
	return false;
}
