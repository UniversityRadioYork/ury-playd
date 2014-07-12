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
#include "io/io_responder.hpp"
#include "messages.h"

CommandHandler &CommandHandler::AddNullary(const std::string &word,
                                           std::function<bool()> f)
{
	this->commands.emplace(word, [f](const WordList &) { return f(); });
	return *this;
}

CommandHandler &CommandHandler::AddUnary(
                const std::string &word,
                std::function<bool(const std::string &)> f)
{
	this->commands.emplace(word, [f](const WordList &words) {
		bool valid = false;
		if (words.size() == 2 && !words[1].empty()) {
			valid = f(words[1]);
		}
		return valid;
	});
	return *this;
}

bool CommandHandler::Run(const CommandHandler::WordList &words)
{
	bool valid = false;

	if (!words.empty()) {
		auto commandIter = this->commands.find(words[0]);
		if (commandIter != this->commands.end()) {
			valid = commandIter->second(words);
		}
	}

	return valid;
}

bool CommandHandler::RunLine(const std::string &line)
{
	return Run(LineToWords(line));
}

bool CommandHandler::Handle(const std::string &line)
{
	Debug("got command: <", line, ">");

	/* Silently fail if the command is actually end of file */
	if (std::cin.eof()) {
		Debug("end of file");
		throw Error("TODO: Handle this better");
	}

	return RunLine(line);
}

CommandHandler::WordList CommandHandler::LineToWords(const std::string &line)
{
	WordList words;

	using Separator = boost::escaped_list_separator<char>;
	using Tokeniser = boost::tokenizer<Separator>;
	Separator separator('\\', ' ', '\"');
	Tokeniser tok(line, separator);

	words.assign(tok.begin(), tok.end());

	return words;
}
