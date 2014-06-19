/*
 * cmd.h - command parser
 * Architecture
 *
 * Contributors:  Matt Windsor <matt.windsor@ury.org.uk>
 */

/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#ifndef PS_CMD_HPP
#define PS_CMD_HPP

#include <memory>
#include <functional>
#include <map>
#include <vector>

#ifdef IGNORE
#undef IGNORE
#endif

typedef std::vector<std::string> cmd_words;
typedef std::function<bool(cmd_words)> payload;
typedef std::map<std::string, payload> command_set;

class CommandHandler {
public:
	CommandHandler(const command_set &commands);
	bool Run(const cmd_words &words);
	bool RunLine(const std::string &line);

private:
	std::unique_ptr<command_set> commands;

	cmd_words LineToWords(const std::string &line);
};

void check_commands(const command_set &cmds);
void handle_cmd(const command_set &cmds);

#endif // PS_CMD_HPP
