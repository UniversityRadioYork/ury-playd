// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the CommandHandler class.
 * @see cmd.cpp
 */

#ifndef PLAYD_CMD_HPP
#define PLAYD_CMD_HPP

#include <map>
#include <memory>
#include <vector>

#include "cmd_result.hpp"
#include "player/player.hpp"

#ifdef IGNORE
#undef IGNORE
#endif

/**
 * The playd command handler.
 */
class CommandHandler
{
public:
	/// The type of lists of command words.
	typedef std::vector<std::string> WordList;

	/**
	 * Constructs a CommandHandler.
	 * @param player A reference to the Player on which the playd
	 *   commands will run.
	 */
	CommandHandler(Player &player);

	/**
	 * Handles a command line.
	 * @param words A reference to the list of words in the command.
	 * @return Whether the command succeeded.
	 */
	CommandResult Handle(const WordList &words);

private:
	/// Reference to the Player on which commands run.
	Player &player;

	/**
	 * Runs a nullary (0-argument) command.
	 * @param cmd The command word.
	 * @return True if the command was successfully found and executed;
	 *   false otherwise.
	 */
	CommandResult RunNullary(const std::string &cmd);

	/**
	 * Runs a unary (1-argument) command.
	 * @param cmd The command word.
	 * @param arg The argument to the command.
	 * @return True if the command was successfully found and executed;
	 *   false otherwise.
	 */
	CommandResult RunUnary(const std::string &cmd, const std::string &arg);
};

#endif // PLAYD_CMD_HPP
