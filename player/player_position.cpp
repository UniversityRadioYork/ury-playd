#include <chrono>
#include "player.hpp"

void Player::SendPositionIfReady()
{
	auto position = this->audio->CurrentPosition<
	                std::chrono::microseconds>();
	if (IsReadyToSendPosition(position)) {
		this->position_listener(position);
		this->position_last = position;
		this->position_last_invalid = false;
	}
}

bool Player::IsReadyToSendPosition(std::chrono::microseconds current_position)
{
	bool ready = false;

	if (this->position_last_invalid) {
		ready = true;
	} else if (this->position_listener != nullptr) {
		auto difference = current_position - this->position_last;
		ready = difference >= this->position_period;
	}

	return ready;
}

void Player::RegisterPositionListener(PositionListener listener,
                                      const std::chrono::microseconds period)
{
	this->position_listener = listener;
	this->position_period = period;
}
