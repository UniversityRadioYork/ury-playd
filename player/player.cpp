/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#include <string>
#include <cassert>

#include "player.hpp"
#include "../audio/audio_system.hpp"
#include "../time_parser.hpp"

Player::Player(const AudioSystem &audio_system, const Player::TP &time_parser)
    : audio_system(audio_system), time_parser(time_parser)
{
	this->current_state = State::EJECTED;
	this->audio = nullptr;
}

void Player::Update()
{
	if (this->current_state == State::PLAYING) {
		if (this->audio->IsHalted()) {
			Eject();
		} else {
			UpdatePosition();
		}
	}
	if (CurrentStateIn({State::PLAYING, State::STOPPED})) {
		if (!this->audio->Update()) {
			Eject();
		}
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
	return IfCurrentStateIn({State::STOPPED, State::PLAYING}, [this] {
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
			error.ToResponse();
			Eject();
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
	return IfCurrentStateIn({State::PLAYING, State::STOPPED},
	                        [this, &time_str] {
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
