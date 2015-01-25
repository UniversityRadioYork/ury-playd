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

/* static */ const Response::Code CommandResult::TYPE_CODES[] = {
        Response::Code::OK,   // Type::SUCCESS
        Response::Code::WHAT, // Type::INVALID
        Response::Code::FAIL, // Type::FAILURE
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
	Response r(CommandResult::TYPE_CODES[static_cast<uint8_t>(this->type)]);

	// Only display a message if the result wasn't a successful one.
	// The message goes at the front, as then clients always know it's the
	// first argument.
	if (this->type != Type::SUCCESS) r.AddArg(this->msg);

	// Then, add in the original command words.
	for (auto &cwd : cmd) r.AddArg(cwd);

	sink.Respond(r);
}
