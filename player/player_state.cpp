// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of aspects of the Player class pertaining to its state.
 * @see player/player.hpp
 * @see player/player.cpp
 * @see player/player_position.hpp
 * @see player/player_position.cpp
 */

#include "player.hpp"

// Basic state queries

Player::State Player::CurrentState() const { return this->current_state; }

bool Player::IsRunning() const { return CurrentState() != State::QUITTING; }

// State conditionals

bool Player::CurrentStateIn(Player::StateList states) const
{
	return std::find(states.begin(), states.end(), this->current_state) !=
	       states.end();
}

bool Player::IfCurrentStateIn(Player::StateList states, std::function<bool()> f)
{
	bool result = false;

	if (CurrentStateIn(states)) {
		result = f();
	}

	return result;
}

// State strings

const std::map<Player::State, std::string> Player::STATE_STRINGS = {
                {State::STARTING, "Starting"},
                {State::EJECTED, "Ejected"},
                {State::STOPPED, "Stopped"},
                {State::PLAYING, "Playing"},
                {State::QUITTING, "Quitting"}};

const std::string& Player::StateString(State state)
{
	return Player::STATE_STRINGS.at(state);
}

const std::string& Player::CurrentStateString() const
{
	return Player::StateString(CurrentState());
}

// Changing state

void Player::SetState(State state)
{
	State last_state = this->current_state;

	this->current_state = state;

	if (this->state_listener != nullptr) {
		this->state_listener(last_state, state);
	}
}

// Listening for state changes

void Player::RegisterStateListener(StateListener listener)
{
	this->state_listener = listener;
}
