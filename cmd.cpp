/* cmd.cpp - command parser */

/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#include <iostream>
#include <string>
#include <vector>

#include <boost/tokenizer.hpp>

#include <ctype.h>
#include <stdbool.h> /* bool */
#include <stdio.h>   /* getline */
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#include "constants.h" /* WORD_LEN */
#include "cmd.h"       /* struct cmd, enum cmd_type */
#include "errors.hpp"
#include "io.hpp"
#include "messages.h" /* Messages (usually errors) */

/**
 * Constructs a CommandHandler.
 * @param commands The map of commands to their handlers to use for this
 *   CommandHandler: this map will be copied.
 */
CommandHandler::CommandHandler(const command_set &commands)
{
	this->commands =
	                std::unique_ptr<command_set>(new command_set(commands));
}

/**
 * Runs a command.
 * @param words The words that form the command: the first word is taken to be
 *   the command name.
 * @return true if the command was valid; false otherwise.
 */
bool CommandHandler::Run(const cmd_words &words)
{
	bool valid = false;

	if (!words.empty()) {
		auto commandIter = this->commands->find(words[0]);
		if (commandIter != this->commands->end()) {
			valid = commandIter->second(words);
		}
	}

	return valid;
}

/**
 * Parses a string as a command line and runs the result.
 * @param line The string that represents the command line.
 * @return true if the command was valid; false otherwise.
 */
bool CommandHandler::RunLine(const std::string &line)
{
	return Run(LineToWords(line));
}

/*
 * Checks to see if there is a command waiting on stdin and, if there is,
 * sends it to the command handler.
 *
 * 'usr' is a pointer to any user data that should be passed to executed
 * commands; 'cmds' is a pointer to an END_CMDS-terminated array of command
 * definitions (see cmd.h for details).
 */
void check_commands(const command_set &cmds)
{
	if (input_waiting()) {
		handle_cmd(cmds);
	}
}
/* Processes the command currently waiting on the given stream.
 * If the command is set to be handled by PROPAGATE, it will be sent through
 * prop; it is an error if prop is NULL and PROPAGATE is reached.
 */
void handle_cmd(const command_set &cmds)
{
	std::string input;

	std::getline(std::cin, input);
	Debug("got command: ", input);

	/* Silently fail if the command is actually end of file */
	if (std::cin.eof()) {
		Debug("end of file");
		throw Error(ErrorCode::END_OF_FILE, "TODO: Handle this better");
	}

	CommandHandler ch = CommandHandler(cmds);
	bool valid = ch.RunLine(input);

	if (valid) {
		Respond(Response::OKAY, input);
	} else {
		Respond(Response::WHAT, "Invalid command.");
	}
}

/**
 * Parses a command line into a list of words.
 * @param line The line to split into words.
 * @return The list of words in the command line.
 */
cmd_words CommandHandler::LineToWords(const std::string &line)
{
	cmd_words words;

	typedef boost::tokenizer<boost::escaped_list_separator<char>> Tokeniser;
	boost::escaped_list_separator<char> separator('\\', ' ', '\"');
	Tokeniser tok(line, separator);

	words.assign(tok.begin(), tok.end());

	return words;
}
