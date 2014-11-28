// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Definition of the CommandResult class.
 * @see cmd_result.hpp
 */

#include <string>
#include <vector>

#include "io/io_response.hpp"
#include "cmd_result.hpp"

/* static */ const ResponseCode CommandResult::TYPE_CODES[] = {
	ResponseCode::OKAY, // Type::SUCCESS
	ResponseCode::WHAT, // Type::INVALID
	ResponseCode::FAIL, // Type::FAILURE
};

CommandResult CommandResult::Success()
{
	return CommandResult(Type::SUCCESS, "success");
}

CommandResult CommandResult::Invalid(const std::string &msg)
{
	return CommandResult(Type::INVALID, msg);
}

CommandResult CommandResult::Failure(const std::string &msg)
{
	return CommandResult(Type::FAILURE, msg);
}

CommandResult::CommandResult(CommandResult::Type type, const std::string &msg)
    : type(type), msg(msg)
{
}

bool CommandResult::IsSuccess() const
{
	return this->type == Type::SUCCESS;
}

void CommandResult::Emit(const ResponseSink &sink,
                         const std::vector<std::string> &cmd) const
{
	std::vector<std::string> args(cmd);

	// Only display a message if the result wasn't a successful one.
	// The message goes at the front, as then clients always know it's the
	// first argument.
	if (this->type != Type::SUCCESS) args.emplace(args.begin(), this->msg);

	auto code = CommandResult::TYPE_CODES[static_cast<uint8_t>(this->type)];
	sink.RespondArgs(code, args);
}
