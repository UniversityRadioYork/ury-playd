// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
* @file
* Implementation of the PlayerState class, and related Player methods.
* @see player/player_position.hpp
* @see player/player.hpp
*/

#include "player.hpp"
#include "player_state.hpp"
#include "../io/io_response.hpp"

//
// Player
//

bool Player::IfCurrentStateIn(PlayerState::List states,
                              PlayerState::StateRestrictedFunction f)
{
	return this->state.IfIn(states, f);
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

const PlayerState::List PlayerState::AUDIO_PLAYING_STATES = {PlayerState::State::PLAYING};

const PlayerState::List PlayerState::AUDIO_LOADED_STATES = {PlayerState::State::PLAYING, PlayerState::State::STOPPED};

PlayerState::PlayerState() : current(State::EJECTED) {};

const void PlayerState::Emit(ResponseSink &responder) const
{
	responder.Respond(ResponseCode::STAT,
	                  PlayerState::STRINGS.at(this->current));
}

const bool PlayerState::IfIn(PlayerState::List states,
	                     PlayerState::StateRestrictedFunction f) const
{
	bool result = false;

	if (In(states)) {
		result = f();
	}

	return result;
}

const bool PlayerState::IsRunning() const
{
	return this->current != State::QUITTING;
}

// Private

const std::map<PlayerState::State, std::string> PlayerState::STRINGS = {
		{ State::STARTING, "Starting" },
		{ State::EJECTED, "Ejected" },
		{ State::STOPPED, "Stopped" },
		{ State::PLAYING, "Playing" },
		{ State::QUITTING, "Quitting" } };

const bool PlayerState::In(PlayerState::List states) const
{
	return std::find(states.begin(), states.end(), this->current) !=
	       states.end();
}

void PlayerState::Set(PlayerState::State state)
{
	this->current = state;
	EmitToRegisteredSink();
}