#include "player.hpp"

#include <cassert>

bool Player::Play()
{
	return IfCurrentStateIn({State::STOPPED}, [this] {
		assert(this->audio != nullptr);
		this->audio->Start();
		SetState(State::PLAYING);
		return true;
	});
}
