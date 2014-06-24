#include "../audio/audio_output.hpp"

#include "player.hpp"
#include <cassert>

bool Player::Load(const std::string &path)
{
	bool valid = !path.empty();
	if (valid) {
		try
		{
			OpenFile(path);
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

void Player::OpenFile(const std::string &path)
{
	this->audio = decltype(this->audio)(this->audio_system.Load(path));
}
