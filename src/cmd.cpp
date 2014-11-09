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
#include "errors.hpp"
#include "io/io_response.hpp"
#include "messages.h"

CommandHandler::CommandHandler(Player &player) : player(player)
{
}

CommandResult CommandHandler::Handle(const CommandHandler::WordList &words)
{
	if (words.size() == 1) {
		return RunNullary(words[0]);
	} else if (words.size() == 2 && !words[1].empty()) {
		return RunUnary(words[0], words[1]);
	}
	return false;
}

CommandResult CommandHandler::RunNullary(const std::string &cmd)
{
	if ("play" == cmd) return this->player.Play();
	if ("stop" == cmd) return this->player.Stop();
	if ("eject" == cmd) return this->player.Eject();
	if ("quit" == cmd) return this->player.Quit();
	return false;
}

CommandResult CommandHandler::RunUnary(const std::string &cmd, const std::string &arg)
{
	if ("load" == cmd) return this->player.Load(arg);
	if ("seek" == cmd) return this->player.Seek(arg);
	return false;
}
