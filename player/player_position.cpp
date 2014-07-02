// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the PlayerPosition class.
 * @see player/player_position.hpp
 */

#include <chrono>
#include "player.hpp"

// Player

void Player::RegisterPositionListener(PlayerPosition::Listener listener)
{
	this->position.RegisterListener(listener);
}

void Player::SetPositionListenerPeriod(PlayerPosition::Unit period)
{
	this->position.SetListenerPeriod(period);
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
	this->last = decltype(this->last)();
}

void PlayerPosition::Send()
{
	for (auto listener : this->listeners) {
		listener(this->current);
	}
	this->last = decltype(this->last)(this->current);
}

bool PlayerPosition::IsReadyToSend()
{
	return (!this->last) || ((*this->last) + this->period <= this->current);
}

void PlayerPosition::RegisterListener(PlayerPosition::Listener listener)
{
	this->listeners.push_back(listener);
}

void PlayerPosition::SetListenerPeriod(PlayerPosition::Unit period)
{
	this->period = period;
}
