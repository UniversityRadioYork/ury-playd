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
#include "../io/io_response.hpp"

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
	// Default to broadcasting the position every time it is changed.
	this->period = decltype(this->period)(0);

	Reset();
}

void PlayerPosition::Update(const PlayerPosition::Unit position)
{
	this->current = position;

	if (IsReadyToSend()) {
		EmitToRegisteredSink();
		this->last = this->current;
	}
}

void PlayerPosition::Reset()
{
	this->current = decltype(this->current)(0);
	this->last = boost::none;
}

const void PlayerPosition::Emit(ResponseSink &target) const
{
	std::ostringstream os;
	os << this->current.count();

	target.Respond(ResponseCode::TIME, os.str());
}

bool PlayerPosition::IsReadyToSend()
{
	return (!this->last) || ((*this->last) + this->period <= this->current);
}

void PlayerPosition::SetResponsePeriod(PlayerPosition::Unit period)
{
	this->period = period;
}
