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

CommandResult CommandResult::Success(const std::string &tag)
{
	return CommandResult(tag, CommandResult::Code::OK, "success");
}

CommandResult CommandResult::Invalid(const std::string &tag, const std::string &msg)
{
	return CommandResult(tag, CommandResult::Code::WHAT, msg);
}

CommandResult CommandResult::Failure(const std::string &tag, const std::string &msg)
{
	return CommandResult(tag, CommandResult::Code::FAIL, msg);
}

CommandResult::CommandResult(const std::string &tag, CommandResult::Code type, const std::string &msg)
    : tag(tag), msg(msg), type(type)
{
}

bool CommandResult::IsSuccess() const
{
	return this->type == CommandResult::Code::OK;
}

void CommandResult::Emit(size_t id, const ResponseSink &sink) const
{
	Response r(this->tag, Response::Code::ACK);
	r.AddArg(CommandResult::STRINGS[static_cast<int>(this->type)]);
	r.AddArg(this->msg);

	sink.Respond(id, r);
}
