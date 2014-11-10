// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the CommandHandler class.
 * @see cmd.hpp
 */

#include <iostream>
#include <string>

#include "cmd.hpp"
#include "cmd_result.hpp"
#include "errors.hpp"
#include "io/io_response.hpp"
#include "messages.h"

CommandHandler::CommandHandler(Player &player) : player(player)
{
}

CommandResult CommandHandler::Handle(const CommandHandler::WordList &words)
{
	if (words.size() == 1)
		return RunNullary(words[0]);

	if (words.size() == 2 && !words[1].empty())
		return RunUnary(words[0], words[1]);

	return CommandResult::Invalid(MSG_CMD_INVALID);
}

CommandResult CommandHandler::RunNullary(const std::string &cmd)
{
	if ("play" == cmd) return this->player.Play();
	if ("stop" == cmd) return this->player.Stop();
	if ("eject" == cmd) return this->player.Eject();
	if ("quit" == cmd) return this->player.Quit();

	return CommandResult::Invalid(MSG_CMD_INVALID);
}

CommandResult CommandHandler::RunUnary(const std::string &cmd, const std::string &arg)
{
	if ("load" == cmd) return this->player.Load(arg);
	if ("seek" == cmd) return this->player.Seek(arg);

	return CommandResult::Invalid(MSG_CMD_INVALID);
}
