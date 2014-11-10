// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the PlayerPosition class, and related Player methods.
 * @see player/player_position.hpp
 * @see player/player.hpp
 */

#include <sstream>

#include "../io/io_response.hpp"
#include "../time_parser.hpp"
#include "player.hpp"
#include "player_position.hpp"

//
// Player
//

void Player::SetPositionResponsePeriod(TimeParser::MicrosecondPosition period)
{
	this->position.SetResponsePeriod(period);
}

void Player::UpdatePosition()
{
	auto pos = this->file.CurrentPosition();
	this->position.Update(pos);
}

void Player::ResetPosition()
{
	this->position.Reset();
}

//
// PlayerPosition
//

// Default to broadcasting the position every time it is changed.
PlayerPosition::PlayerPosition() : ResponseSource(), period(0)
{
	Reset();
}

void PlayerPosition::Update(const TimeParser::MicrosecondPosition position)
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
	this->current = TimeParser::MicrosecondPosition(0);
	this->last = TimeParser::MicrosecondPosition(0);
	this->has_reset = true;
}

void PlayerPosition::Emit(ResponseSink &target) const
{
	std::ostringstream os;
	os << this->current;

	target.Respond(ResponseCode::TIME, os.str());
}

bool PlayerPosition::IsReadyToSend() const
{
	return this->has_reset || (this->last + this->period <= this->current);
}

void PlayerPosition::SetResponsePeriod(TimeParser::MicrosecondPosition period)
{
	this->period = period;
}
