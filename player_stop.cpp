#include "player.hpp"
#include "cmd.hpp"

CommandHandler::NullAction Player::StopAction()
{
	return [this] { return this->Stop(); };
}

bool Player::Stop()
{
	return IfCurrentStateIn({State::PLAYING}, [this] {
		this->au->Stop();
		SetState(State::STOPPED);
	});
}
