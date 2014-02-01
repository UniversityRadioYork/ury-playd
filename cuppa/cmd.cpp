/*******************************************************************************
 * cmd.c - command parser
 *   Part of cuppa, the Common URY Playout Package Architecture
 *
 * Contributors:  Matt Windsor <matt.windsor@ury.org.uk>
 */

/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
 */

#define _POSIX_C_SOURCE 200809

#include <iostream>
#include <string>
#include <vector>

#include <ctype.h>
#include <stdbool.h>		/* bool */
#include <stdio.h>		/* getline */
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#include "constants.h"		/* WORD_LEN */
#include "cmd.h"		/* struct cmd, enum cmd_type */
#include "errors.h"		/* error */
#include "io.h"			/* response */
#include "messages.h"		/* Messages (usually errors) */

/**
 * Constructs a CommandHandler.
 * @param commands The map of commands to their handlers to use for this
 *   CommandHandler: this map will be copied.
 */
CommandHandler::CommandHandler(const command_set &commands)
{
	this->commands = std::unique_ptr<command_set>(new command_set(commands));
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

	auto commandIter = this->commands->find(words[0]);
	if (commandIter != this->commands->end()) {
		valid = commandIter->second(words);
	}

	return valid;
}

static cmd_words tokenise(const std::string &line);

/*
 * Checks to see if there is a command waiting on stdin and, if there is,
 * sends it to the command handler.
 *
 * 'usr' is a pointer to any user data that should be passed to executed
 * commands; 'cmds' is a pointer to an END_CMDS-terminated array of command
 * definitions (see cmd.h for details).
 */
enum error
check_commands(const command_set &cmds)
{
	enum error	err = E_OK;

	if (input_waiting()) {
		err = handle_cmd(cmds);
	}

	return err;
}
/* Processes the command currently waiting on the given stream.
 * If the command is set to be handled by PROPAGATE, it will be sent through
 * prop; it is an error if prop is NULL and PROPAGATE is reached.
 */
enum error
handle_cmd(const command_set &cmds)
{
	std::string input;
	enum error	err = E_OK;
	cmd_words words;
	char           *arg = NULL;
	char           *end = NULL;
	size_t		num_bytes = 0;

	std::getline(std::cin, input);
	dbug("got command: %s", input.c_str());

	/* Silently fail if the command is actually end of file */
	if (std::cin.eof()) {
		dbug("end of file");
		err = E_EOF;
	}
	if (err == E_OK) {
		words = tokenise(input);
		if (words.empty())
			err = error(E_BAD_COMMAND, MSG_CMD_NOWORD);
	}
	if (err == E_OK) {
		CommandHandler ch = CommandHandler(cmds);
		bool valid = ch.Run(words);

		if (valid) {
			response(R_OKAY, input.c_str());
		}
		else {
			error(E_BAD_COMMAND, "Bad command (or file name?)");
		}
	}
	dbug("command processed");

	return err;
}

static std::vector<std::string>
tokenise(const std::string &line)
{
	unsigned int size = line.size();
	cmd_words words = {};

	std::string current_word = "";

	for (char c : line) {
		if (c == ' ' && !(current_word.empty())) {
			words.push_back(std::string(current_word));
			current_word.clear();
		}
		else {
			current_word.push_back(c);
		}
	}
	if (!current_word.empty()) {
		words.push_back(std::string(current_word));
	}

	return words;
}
