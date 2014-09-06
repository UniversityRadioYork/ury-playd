// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

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
// Player
//

bool Player::CurrentStateIn(PlayerState::List states) const
{
	return this->state.In(states);
}

bool Player::IsRunning() const
{
	return this->state.IsRunning();
}

void Player::SetState(PlayerState::State state)
{
	this->state.Set(state);
}

//
// PlayerState
//

const PlayerState::List PlayerState::AUDIO_PLAYING_STATES = {
	PlayerState::State::PLAYING
};

const PlayerState::List PlayerState::AUDIO_LOADED_STATES = {
	PlayerState::State::PLAYING, PlayerState::State::STOPPED
};

PlayerState::PlayerState() : current(State::EJECTED)
{
}

void PlayerState::Emit(ResponseSink &responder) const
{
	responder.Respond(ResponseCode::STATE,
	                  PlayerState::STRINGS.at(this->current));
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

const std::map<PlayerState::State, std::string> PlayerState::STRINGS = {
	{ State::STARTING, "Starting" },
	{ State::EJECTED, "Ejected" },
	{ State::STOPPED, "Stopped" },
	{ State::PLAYING, "Playing" },
	{ State::QUITTING, "Quitting" }
};

void PlayerState::Set(PlayerState::State state)
{
	this->current = state;
	Push();
}
