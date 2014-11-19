// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the PlayerState class, and related Player methods.
 * @see player/player_state.hpp
 * @see player/player.hpp
 */

#include <algorithm>
#include <array>
#include <cassert>

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

PlayerState::PlayerState() : ResponseSource(), current(State::EJECTED)
{
}

void PlayerState::Emit(ResponseSink &responder) const
{
	auto c = static_cast<uint8_t>(this->current);
	assert(c < PlayerState::STATE_COUNT);
	responder.Respond(ResponseCode::STATE,
	                  PlayerState::STATE_STRINGS.at(c));
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

void PlayerState::Set(PlayerState::State state)
{
	this->current = state;
	this->Push();
}

const std::array<std::string, PlayerState::STATE_COUNT>
                PlayerState::STATE_STRINGS = { {
		        "Starting", // State::STARTING
		        "Ejected",  // State::EJECTED
		        "Stopped",  // State::STOPPED
		        "Playing",  // State::PLAYING
		        "Quitting", // State::QUITTING
		} };
