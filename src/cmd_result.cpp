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

const std::string CommandResult::STRINGS[] = {
        "OK",   // Code::OK
        "WHAT", // Code::WHAT
        "FAIL"  // Code::FAIL
};

CommandResult CommandResult::Success()
{
	return CommandResult(CommandResult::Code::OK, "success");
}

CommandResult CommandResult::Invalid(const std::string &msg)
{
	return CommandResult(CommandResult::Code::WHAT, msg);
}

CommandResult CommandResult::Failure(const std::string &msg)
{
	return CommandResult(CommandResult::Code::FAIL, msg);
}

CommandResult::CommandResult(CommandResult::Code type, const std::string &msg)
    : msg(msg), type(type)
{
}

bool CommandResult::IsSuccess() const
{
	return this->type == CommandResult::Code::OK;
}

void CommandResult::Emit(const ResponseSink &sink,
                         const std::vector<std::string> &cmd, size_t id) const
{
	Response r(Response::Code::ACK);
	r.AddArg(CommandResult::STRINGS[static_cast<int>(this->type)]);
	r.AddArg(this->msg);
	for (auto &cwd : cmd) r.AddArg(cwd);

	sink.Respond(r, id);
}
