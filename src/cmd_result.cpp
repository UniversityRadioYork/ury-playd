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

bool CommandResult::IsSuccess()
{
	return this->type == Type::SUCCESS;
}

void CommandResult::Emit(const ResponseSink &sink,
                         const std::vector<std::string> &cmd)
{
	switch (this->type) {
		case Type::SUCCESS:
			sink.RespondArgs(ResponseCode::OKAY, cmd);
			break;
		case Type::INVALID:
			sink.Respond(ResponseCode::WHAT, this->msg);
			break;
		case Type::FAILURE:
			sink.Respond(ResponseCode::FAIL, this->msg);
			break;
	}
}
