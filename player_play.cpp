#include "player.hpp"
#include "cmd.hpp"

CommandHandler::NullAction Player::PlayAction()
{
	return [this] { return this->Play(); };
}

bool Player::Play()
{
	return IfCurrentStateIn({State::STOPPED}, [this] {
		assert(this->audio != nullptr);
		this->audio->Start();
		SetState(State::PLAYING);
	});
}
