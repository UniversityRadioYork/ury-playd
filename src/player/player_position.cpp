// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the PlayerPosition class, and related Player methods.
 * @see player/player_position.hpp
 * @see player/player.hpp
 */

#include <cstdint>
#include <sstream>

#include "../io/io_response.hpp"
#include "player.hpp"
#include "player_position.hpp"

//
// PlayerPosition
//

PlayerPosition::PlayerPosition(const ResponseSink *time_sink,
                               std::uint64_t period)
    : ResponseSource(time_sink), period(period)
{
	this->Reset();
}

void PlayerPosition::Update(const std::uint64_t position)
{
	this->current = position;

	if (this->IsReadyToSend()) {
		this->Push();
		this->last = this->current;
		this->has_reset = 0;
	}
}

void PlayerPosition::Reset()
{
	this->current = 0;
	this->last = 0;
	this->has_reset = true;
}

void PlayerPosition::Emit(const ResponseSink &target) const
{
	target.Respond(Response(Response::Code::TIME).Arg(std::to_string(this->current)));
}

bool PlayerPosition::IsReadyToSend() const
{
	return this->has_reset || (this->last + this->period <= this->current);
}
