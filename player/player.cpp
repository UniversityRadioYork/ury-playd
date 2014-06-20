/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#include <memory>
#include <vector>

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#include <algorithm>
#include <thread>
#include <chrono>

#ifdef WIN32
struct timespec {
	time_t tv_sec;
	long tv_nsec;
};
#include <Winsock2.h>
#else
#include <time.h> /* struct timespec */
#endif

#include "player.hpp"

#include "../audio.hpp"
#include "../cmd.hpp"
#include "../constants.h"
#include "../io.hpp"
#include "../messages.h"

Player::Player(const std::string& device) : device(device)
{
	this->current_state = State::EJECTED;
	this->audio = nullptr;

	this->position_listener = nullptr;
	this->position_period = std::chrono::microseconds(0);
	this->position_last = std::chrono::microseconds(0);
	this->position_last_invalid = true;
}


void Player::Update()
{
	if (this->current_state == State::PLAYING) {
		if (this->audio->IsHalted()) {
			Eject();
		} else {
			SendPositionIfReady();
		}
	}
	if (CurrentStateIn({State::PLAYING, State::STOPPED})) {
		if (!this->audio->Update()) {
			Eject();
		}
	}
}


void Player::SendPositionIfReady()
{
	auto position = this->audio->CurrentPosition<
	                std::chrono::microseconds>();
	if (IsReadyToSendPosition(position)) {
		this->position_listener(position);
		this->position_last = position;
		this->position_last_invalid = false;
	}
}

bool Player::IsReadyToSendPosition(std::chrono::microseconds current_position)
{
	bool ready = false;

	if (this->position_last_invalid) {
		ready = true;
	} else if (this->position_listener != nullptr) {
		auto difference = current_position - this->position_last;
		ready = difference >= this->position_period;
	}

	return ready;
}

void Player::RegisterPositionListener(PositionListener listener,
                                      const std::chrono::microseconds period)
{
	this->position_listener = listener;
	this->position_period = period;
}
