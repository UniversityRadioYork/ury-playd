// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the PlayerPosition class, and related Player methods.
 * @see player/player_position.hpp
 * @see player/player.hpp
 */

#include <chrono>
#include <sstream>
#include "player.hpp"
#include "../io/io_responder.hpp"

// Player

void Player::SetPositionResponsePeriod(PlayerPosition::Unit period)
{
	this->position.SetResponsePeriod(period);
}

void Player::UpdatePosition()
{
	auto pos = this->audio->CurrentPosition<PlayerPosition::Unit>();
	this->position.Update(pos);
}

void Player::ResetPosition()
{
	this->position.Reset();
}

// PlayerPosition
PlayerPosition::PlayerPosition()
{
	this->listeners = decltype(this->listeners)();

	// Default to broadcasting the position every time it is changed.
	this->period = decltype(this->period)(0);

	Reset();
}

void PlayerPosition::Update(const PlayerPosition::Unit position)
{
	this->current = position;

	if (IsReadyToSend()) {
		Send();
	}
}

void PlayerPosition::Reset()
{
	this->current = decltype(this->current)(0);
	this->last = boost::none;
}

void PlayerPosition::Emit(Responder &target)
{
	std::ostringstream os;
	os << this->current.count();

	target.Respond(Response::TIME, os.str());
}

void PlayerPosition::Send()
{
	for (auto listener : this->listeners) {
		Emit(listener);
	}
	this->last = this->current;
}

bool PlayerPosition::IsReadyToSend()
{
	return (!this->last) || ((*this->last) + this->period <= this->current);
}

void PlayerPosition::SetResponder(Responder &listener)
{
	this->listeners.push_back(listener);
}

void PlayerPosition::SetResponsePeriod(PlayerPosition::Unit period)
{
	this->period = period;
}
