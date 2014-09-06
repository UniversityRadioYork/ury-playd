// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the PlayerPosition class, and related Player methods.
 * @see player/player_position.hpp
 * @see player/player.hpp
 */

#include <chrono>
#include <sstream>

#include "../io/io_response.hpp"
#include "player.hpp"

//
// Player
//

void Player::SetPositionResponsePeriod(PlayerPosition::Unit period)
{
	this->position.SetResponsePeriod(period);
}

void Player::UpdatePosition()
{
	auto pos = this->file.CurrentPosition<PlayerPosition::Unit>();
	this->position.Update(pos);
}

void Player::ResetPosition()
{
	this->position.Reset();
}

//
// PlayerPosition
//

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
		Push();
		this->last = this->current;
		this->has_reset = 0;
	}
}

void PlayerPosition::Reset()
{
	this->current = Unit(0);
	this->last = Unit(0);
	this->has_reset = true;
}

void PlayerPosition::Emit(ResponseSink &target) const
{
	std::ostringstream os;
	os << this->current.count();

	target.Respond(ResponseCode::TIME, os.str());
}

bool PlayerPosition::IsReadyToSend()
{
	return this->has_reset || (this->last + this->period <= this->current);
}

void PlayerPosition::SetResponsePeriod(PlayerPosition::Unit period)
{
	this->period = period;
}
