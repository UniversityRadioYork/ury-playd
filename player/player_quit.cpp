#include "player.hpp"
#include "../cmd.hpp"

CommandHandler::NullAction Player::QuitAction()
{
	return [this] { return this->Quit(); };
}

bool Player::Quit()
{
	Eject();
	SetState(State::QUITTING);

	return true; // Always a valid command.
}
