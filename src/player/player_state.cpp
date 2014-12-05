// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the PlayerState class, and related Player methods.
 * @see player/player_state.hpp
 * @see player/player.hpp
 */

#include <algorithm>

#include "../io/io_response.hpp"
#include "player_state.hpp"
#include "player.hpp"

//
// PlayerState
//

const PlayerState::List PlayerState::AUDIO_PLAYING_STATES = {
	PlayerState::State::PLAYING
};

const PlayerState::List PlayerState::AUDIO_LOADED_STATES = {
	PlayerState::State::PLAYING, PlayerState::State::STOPPED
};

PlayerState::PlayerState() : ResponseSource(), current(State::EJECTED)
{
}

void PlayerState::Emit(ResponseSink &responder) const
{
	auto c = static_cast<int>(this->current);
	responder.Respond(ResponseCode::STATE, PlayerState::STRINGS[c]);
}

bool PlayerState::In(PlayerState::List states) const
{
	return std::find(states.begin(), states.end(), this->current) !=
	       states.end();
}

bool PlayerState::IsRunning() const
{
	return this->current != State::QUITTING;
}

// Private

const std::string PlayerState::STRINGS[] = {
	"Starting", // State::STARTING
	"Ejected",  // State::EJECTED
	"Stopped",  // State::STOPPED
	"Playing",  // State::PLAYING
	"Quitting", // State::QUITTING
};

void PlayerState::Set(PlayerState::State state)
{
	this->current = state;
	this->Push();
}
