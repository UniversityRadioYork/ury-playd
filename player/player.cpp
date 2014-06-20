/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#include <string>

#include "player.hpp"
#include "../audio.hpp"

Player::Player(const std::string& device) : device(device)
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
