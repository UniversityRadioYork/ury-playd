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

class CommandHandler {
public:
	using WordList = std::vector<std::string>;
	using Payload = std::function<bool(WordList)>;
	using CommandSet = std::map<std::string, Payload>;

	using NullAction = std::function<bool()>;
	using SingleRequiredWordAction = std::function<bool(std::string &)>;

	CommandHandler(const CommandSet &commands);
	void Check();

	static Payload NullCommand(NullAction f);
	static Payload SingleRequiredWordCommand(SingleRequiredWordAction f);

private:
	std::unique_ptr<CommandSet> commands;

	WordList LineToWords(const std::string &line);

	bool Run(const WordList &words);
	bool RunLine(const std::string &line);
	void Handle();
};

#endif // PS_CMD_HPP
