#include "player.hpp"

bool Player::Eject()
{
	return IfCurrentStateIn({State::STOPPED, State::PLAYING}, [this] {
		this->audio = nullptr;
		SetState(State::EJECTED);
		return true;
	});
}
