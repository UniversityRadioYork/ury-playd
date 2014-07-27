// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file player/player.cpp
 * Main implementation file for the Player class.
 * @see player/player.hpp
 * @see player/player_position.cpp
 * @see player/player_state.cpp
 */

#include <stdexcept>
#include <sstream>
#include <string>
#include <cassert>

#include "player.hpp"
#include "player_state.hpp"
#include "../audio/audio.hpp"
#include "../audio/audio_system.hpp"
#include "../errors.hpp"
#include "../io/io_response.hpp"
#include "../messages.h"

Player::Player(const AudioSystem &audio_system, const Player::TP &time_parser)
	: audio_system(audio_system), time_parser(time_parser), position(), state(), audio(nullptr)
{
}

bool Player::Update()
{
	IfCurrentStateIn(PlayerState::AUDIO_PLAYING_STATES,
	                 std::bind(&Player::PlaybackUpdate, this));
	IfCurrentStateIn(PlayerState::AUDIO_LOADED_STATES,
	                 std::bind(&Audio::Update, this->audio.get()));
	return IsRunning();
}

bool Player::PlaybackUpdate()
{
	if (this->audio->IsStopped()) {
		Eject();
	} else {
		UpdatePosition();
	}
	return true;
}

void Player::OpenFile(const std::string &path)
{
	this->audio = decltype(this->audio)(this->audio_system.Load(path));
}

void Player::WelcomeClient(ResponseSink &client)
{
        client.Respond(ResponseCode::OHAI, MSG_OHAI);
        this->state.Emit(client);
        this->position.Emit(client);
}

//
// Commands
//

bool Player::Eject()
{
	return IfCurrentStateIn(PlayerState::AUDIO_LOADED_STATES, [this] {
		this->audio = nullptr;
		SetState(PlayerState::State::EJECTED);
		return true;
	});
}

bool Player::Load(const std::string &path)
{
	bool valid = !path.empty();
	if (valid) {
		try
		{
			OpenFile(path);
			ResetPosition();
			Debug("Loaded ", path);
			SetState(PlayerState::State::STOPPED);
		}
		catch (FileError &)
		{
			// File errors aren't fatal, so catch them here.
			Eject();
			valid = false;
		}
		catch (Error &error)
		{
			// Ensure a load failure doesn't leave a corrupted track
			// loaded.
			Eject();
			throw error;
		}
	}
	return valid;
}

bool Player::Play()
{
	return IfCurrentStateIn({PlayerState::State::STOPPED}, [this] {
		assert(this->audio != nullptr);
		this->audio->Start();
		SetState(PlayerState::State::PLAYING);
		return true;
	});
}

bool Player::Quit()
{
	Eject();
	SetState(PlayerState::State::QUITTING);

	return true; // Always a valid command.
}

bool Player::Seek(const std::string &time_str)
{
	return IfCurrentStateIn(PlayerState::AUDIO_LOADED_STATES, [this, &time_str] {
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
			this->audio->SeekToPosition(position);
			this->ResetPosition();
			this->UpdatePosition();
		}

		return success;
	});
}

void Player::SetResponseSink(ResponseSink &responder)
{
	this->position.SetResponseSink(responder);
	this->state.SetResponseSink(responder);
}

bool Player::Stop()
{
	return IfCurrentStateIn({PlayerState::State::PLAYING }, [this] {
		this->audio->Stop();
		SetState(PlayerState::State::STOPPED);
		return true;
	});
}
