#include "player.hpp"
#include "../cmd.hpp"

CommandHandler::NullAction Player::EjectAction()
{
	return [this] { return this->Eject(); };
}

bool Player::Eject()
{
	return IfCurrentStateIn({State::STOPPED, State::PLAYING}, [this] {
		this->audio = nullptr;
		SetState(State::EJECTED);
	});
}
