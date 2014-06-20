#include "player.hpp"
#include "../cmd.hpp"

CommandHandler::NullAction Player::StopAction()
{
	return [this] { return this->Stop(); };
}

bool Player::Stop()
{
	return IfCurrentStateIn({State::PLAYING}, [this] {
		this->audio->Stop();
		SetState(State::STOPPED);
	});
}
