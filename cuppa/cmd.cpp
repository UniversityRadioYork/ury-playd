/*******************************************************************************
 * cmd.c - command parser
 *   Part of cuppa, the Common URY Playout Package Architecture
 *
 * Contributors:  Matt Windsor <matt.windsor@ury.org.uk>
 */

/*-
 * Copyright (c) 2012, University Radio York Computing Team
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
#include "utils.h"		/* skip_space, nullify_space, skip_unspace */

static bool exec_cmd(const command_set &cmds, const cmd_words &words);
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
		bool valid = exec_cmd(cmds, words);

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

static bool
exec_cmd(const command_set &cmds, const cmd_words &words)
{
	bool valid = false;

	auto cmd = cmds.at(words[0]);
	if (cmd != nullptr) {
		valid = cmd(words);
	}

	return valid;
}
