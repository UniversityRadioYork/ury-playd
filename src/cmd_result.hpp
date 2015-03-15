// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the CommandResult class.
 */

#ifndef PLAYD_CMD_RESULT
#define PLAYD_CMD_RESULT

#include <cstdint>
#include <string>
#include <vector>
#include "response.hpp"

/**
 * A result from a command.
 *
 * Commands may either succeed (with no failure message), or fail (with a
 * failure message).  The success (or not) of a command may be checked with
 * CommandResult::IsSuccess.
 */
class CommandResult
{
public:
	/**
	 * Shortcut for constructing a successful CommandResult.
	 * @return A CommandResult denoting success.
	 */
	static CommandResult Success();

	/**
	 * Shortcut for constructing an invalid-response CommandResult.
	 * @param msg The failure message.  See CommandResult().
	 * @return A CommandResult denoting an invalid response.
	 */
	static CommandResult Invalid(const std::string &msg);

	/**
	 * Shortcut for constructing a failed CommandResult.
	 * @param msg The failure message.  See CommandResult().
	 * @return A CommandResult denoting failure.
	 */
	static CommandResult Failure(const std::string &msg);

	/**
	 * Constructs a CommandResult.
	 * @param type The type of command result, encoded as one of
	 *   the FAIL, WHAT, or OK response codes.
	 * @param msg A message providing more information about the result.
	 *   This will typically only be used for failures.
	 *   The message will be copied into the CommandResult.
	 */
	CommandResult(Response::Code type, const std::string &msg);

	/**
	 * Determines whether this CommandResult was successful.
	 * @return True if the result was a success; false otherwise.
	 */
	bool IsSuccess() const;

	/**
	 * Sends a response to a ResponseSink about this CommandResult.
	 *
	 * If the CommandResult was a success, then the response is 'OK cmd',
	 * where 'cmd' is the parameter named as such.  Otherwise, the response
	 * is 'X msg', where 'msg' is the result message and 'X' is whichever
	 * response code is most appropriate for the failure.
	 *
	 * @param sink The ResponseSink to which the response will be sent.
	 * @param cmd The original command that created this CommandResult.
	 * @param id The connection ID in the sink to which the response will
	 *   be sent.  Defaults to 0 (broadcast).
	 */
	void Emit(const ResponseSink &sink, const std::vector<std::string> &cmd,
	          size_t id = 0) const;

private:
	std::string msg;     ///< The command result's message.
	Response::Code type; ///< The command result's response code.
};

#endif // PLAYD_CMD_RESULT
