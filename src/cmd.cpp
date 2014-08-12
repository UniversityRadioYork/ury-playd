// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the CommandHandler class.
 * @see cmd.hpp
 */

#include <iostream>
#include <string>

#include <boost/tokenizer.hpp>

#include "cmd.hpp"
#include "errors.hpp"
#include "io/io_response.hpp"
#include "messages.h"

CommandHandler::CommandHandler(Player &player) : player(player)
{
}

bool CommandHandler::Run(const CommandHandler::WordList &words)
{
	if (words.size() == 1) {
		return RunNullary(words[0]);
	} else if (words.size() == 2 && !words[1].empty()) {
		return RunUnary(words[0], words[1]);
	}
	return false;
}

bool CommandHandler::RunNullary(const std::string &cmd)
{
	if ("play" == cmd) return this->player.Play();
	if ("stop" == cmd) return this->player.Stop();
	if ("eject" == cmd) return this->player.Eject();
	if ("quit" == cmd) return this->player.Quit();
	return false;
}

bool CommandHandler::RunUnary(const std::string &cmd, const std::string &arg)
{
	if ("load" == cmd) return this->player.Load(arg);
	if ("seek" == cmd) return this->player.Seek(arg);
	return false;
}

bool CommandHandler::Handle(const std::string &line)
{
	Debug() << "got command: <" << line << ">" << std::endl;
	return Run(LineToWords(line));
}

CommandHandler::WordList CommandHandler::LineToWords(const std::string &line)
{
	// See http://git.io/IxXOSg for a description of what this needs to do.

	WordList words;

	using Separator = boost::escaped_list_separator<char>;
	using Tokeniser = boost::tokenizer<Separator>;
	Separator separator('\\', ' ', '\"');
	Tokeniser tok(line, separator);

	words.assign(tok.begin(), tok.end());

	return words;
}
