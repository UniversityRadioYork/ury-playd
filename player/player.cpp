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
#include <string>
#include <cassert>

#include "player.hpp"
#include "../audio/audio_output.hpp"
#include "../audio/audio_system.hpp"
#include "../errors.hpp"

/// List of states in which some audio is loaded.
const Player::StateList Player::AUDIO_LOADED_STATES = {State::PLAYING,
                                                       State::STOPPED};

Player::Player(const AudioSystem &audio_system, const Player::TP &time_parser)
    : audio_system(audio_system), time_parser(time_parser)
{
	this->current_state = State::EJECTED;
	this->audio = nullptr;
}

void Player::Update()
{
	if (this->current_state == State::PLAYING) {
		if (this->audio->IsStopped()) {
			Eject();
		} else {
			UpdatePosition();
		}
	}
	if (CurrentStateIn(AUDIO_LOADED_STATES)) {
		this->audio->Update();
	}
}

void Player::OpenFile(const std::string &path)
{
	this->audio = decltype(this->audio)(this->audio_system.Load(path));
}

//
// Commands
//

bool Player::Eject()
{
	return IfCurrentStateIn(AUDIO_LOADED_STATES, [this] {
		this->audio = nullptr;
		SetState(State::EJECTED);
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
			SetState(State::STOPPED);
		}
		catch (Error &error)
		{
			Eject();
                        throw error;
		}
	}
	return valid;
}

bool Player::Play()
{
	return IfCurrentStateIn({State::STOPPED}, [this] {
		assert(this->audio != nullptr);
		this->audio->Start();
		SetState(State::PLAYING);
		return true;
	});
}

bool Player::Quit()
{
	Eject();
	SetState(State::QUITTING);

	return true; // Always a valid command.
}

bool Player::Seek(const std::string &time_str)
{
	return IfCurrentStateIn(AUDIO_LOADED_STATES, [this, &time_str] {
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

bool Player::Stop()
{
	return IfCurrentStateIn({State::PLAYING}, [this] {
		this->audio->Stop();
		SetState(State::STOPPED);
		return true;
	});
}
