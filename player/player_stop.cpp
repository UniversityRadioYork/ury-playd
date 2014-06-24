#include "player.hpp"

bool Player::Stop()
{
	return IfCurrentStateIn({State::PLAYING}, [this] {
		this->audio->Stop();
		SetState(State::STOPPED);
		return true;
	});
}
