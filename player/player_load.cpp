#include "player.hpp"
#include "../audio.hpp"
#include "../cmd.hpp"

CommandHandler::SingleRequiredWordAction Player::LoadAction()
{
	return [this](const std::string &path) { return this->Load(path); };
}

bool Player::Load(const std::string &filename)
{
	if (filename.length() == 0) return false;

	try
	{
		this->audio = std::unique_ptr<AudioOutput>(
		                new AudioOutput(filename, this->device));
		ResetPosition();
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
