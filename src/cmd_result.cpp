// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Definition of the CommandResult class.
 * @see cmd_result.hpp
 */

#include <string>
#include <vector>

#include "response.hpp"
#include "cmd_result.hpp"

CommandResult CommandResult::Success()
{
	return CommandResult(Response::Code::OK, "success");
}

CommandResult CommandResult::Invalid(const std::string &msg)
{
	return CommandResult(Response::Code::WHAT, msg);
}

CommandResult CommandResult::Failure(const std::string &msg)
{
	return CommandResult(Response::Code::FAIL, msg);
}

CommandResult::CommandResult(Response::Code type, const std::string &msg)
    : msg(msg), type(type)
{
}

bool CommandResult::IsSuccess() const
{
	return this->type == Response::Code::OK;
}

void CommandResult::Emit(const ResponseSink &sink,
                         const std::vector<std::string> &cmd, size_t id) const
{
	Response r(this->type);

	// Only display a message if the result wasn't a successful one.
	// The message goes at the front, as then clients always know it's the
	// first argument.
	if (!this->IsSuccess()) r.AddArg(this->msg);

	// Then, add in the original command words.
	for (auto &cwd : cmd) r.AddArg(cwd);

	sink.Respond(r, id);
}
