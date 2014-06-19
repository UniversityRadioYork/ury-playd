#include "player.hpp"
#include "audio.hpp"
#include "cmd.hpp"

CommandHandler::SingleRequiredWordAction Player::LoadAction()
{
	return [this] (std::string &filename) { return this->Load(filename); };
}

bool Player::Load(const std::string &filename)
{
	if (filename.length() == 0) return false;

	try
	{
		this->au = std::unique_ptr<AudioOutput>(
		                new AudioOutput(filename, this->device));
		this->position_last_invalid = true;
		Debug("Loaded ", filename);
		SetState(State::STOPPED);
	}
	catch (Error &error)
	{
		error.ToResponse();
		Eject();
	}

	return true; // Always a valid command.
}
