// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the CommandHandler class.
 * @see cmd.cpp
 */

#ifndef PS_CMD_HPP
#define PS_CMD_HPP

#include <memory>
#include <functional>
#include <map>
#include <vector>

#include "player/player.hpp"

#ifdef IGNORE
#undef IGNORE
#endif

/**
 * The Playslave++ command handler.
 */
class CommandHandler {
public:
	/// The type of lists of command words.
	using WordList = std::vector<std::string>;

	/**
	 * Constructs a CommandHandler.
	 * @param player A reference to the Player on which the Playslave
	 *   commands will run.
	 */
	CommandHandler(Player &player);

	/**
	 * Handles a command line.
	 * @param words A reference to the list of words in the command.
	 * @return Whether the command succeeded.
	 */
	bool Handle(const WordList &words);

private:
	/// Reference to the Player on which commands run.
	Player &player;

	bool RunNullary(const std::string &cmd);
	bool RunUnary(const std::string &cmd, const std::string &arg);
};

#endif // PS_CMD_HPP
