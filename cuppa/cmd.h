/*
 * cmd.h - command parser Part of cuppa, the Common URY Playout Package
 * Architecture
 *
 * Contributors:  Matt Windsor <matt.windsor@ury.org.uk>
 */

/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
 */

#ifndef CUPPA_CMD_H
#define CUPPA_CMD_H

#include <memory>
#include <functional>
#include <unordered_map>


#ifdef IGNORE
#undef IGNORE
#endif

typedef std::vector<std::string> cmd_words;
typedef std::function <bool(cmd_words)> payload;
typedef std::unordered_map<std::string, payload> command_set;

class CommandHandler {
public:
	CommandHandler(const command_set &commands);
	bool Run(const cmd_words &words);

private:
	std::unique_ptr<command_set> commands;
};

enum error	check_commands(const command_set &cmds);
enum error	handle_cmd(const command_set &cmds);

#endif				/* !CUPPA_CMD_H */
