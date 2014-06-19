#include "player.hpp"
#include "cmd.hpp"

CommandHandler::NullAction Player::EjectAction()
{
	return [this] { return this->Eject(); };
}

bool Player::Eject()
{
	return IfCurrentStateIn({State::STOPPED, State::PLAYING}, [this] {
		this->au = nullptr;
		SetState(State::EJECTED);
		this->position_last = std::chrono::microseconds(0);
	});
}
