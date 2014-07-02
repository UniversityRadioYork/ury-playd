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

	/// The type of functions called on receipt of commands.
	using Payload = std::function<bool(WordList)>;

	/// The type of a set of commands.
	using CommandSet = std::map<std::string, Payload>;

	/// The type of a command action that takes no command words.
	using NullAction = std::function<bool()>;

	/// The type of a command action that takes exactly one command word.
	using SingleRequiredWordAction =
	                std::function<bool(const std::string &)>;

	/**
	 * Constructs a CommandHandler with no arguments.
	 */
	CommandHandler() : CommandHandler(CommandSet{}) {};

	/**
	 * Constructs a CommandHandler with an existing command set.
	 * @param commands The command set, which will be copied.
	 */
	CommandHandler(const CommandSet &commands);

	/**
	 * Handles a command line.
	 * @param line A reference to the line to handle as a command.
	 * @return Whether the command succeeded.
	 */
	bool Handle(const std::string &line);

	/**
	 * Adds a nullary command.
	 * @param word The command word to associate with @a f.
	 * @param f The command, taking no arguments, to execute when the command
	 *   word @a word is read.
	 * @return A pointer to this CommandHandler, for method chaining.
	 */
	CommandHandler *Add(const std::string &word, std::function<bool()> f);

	/**
	 * Adds a unary command.
	 * @param word The command word to associate with @a f.
	 * @param f The command, taking one argument, to execute when the command
	 *   word @a word is read.
	 * @return A pointer to this CommandHandler, for method chaining.
	 */
	CommandHandler *Add(const std::string &word,
	                    std::function<bool(const std::string &)> f);

private:
	std::unique_ptr<CommandSet> commands;

	WordList LineToWords(const std::string &line);

	bool Run(const WordList &words);
	bool RunLine(const std::string &line);
};

#endif // PS_CMD_HPP
