#include "../audio/audio_output.hpp"
#include "../cmd.hpp"

#include "player.hpp"
#include <cassert>

CommandHandler::SingleRequiredWordAction Player::LoadAction()
{
	return [this](const std::string &path) { return this->Load(path); };
}

bool Player::Load(const std::string &path)
{
	bool valid = !path.empty();
	if (valid) {
		try
		{
			this->audio = decltype(this->audio)(this->audio_system.Load(path));
			ResetPosition();
			Debug("Loaded ", path);
			SetState(State::STOPPED);
		}
		catch (Error &error)
		{
			error.ToResponse();
			Eject();
		}
	}
	return valid;
}
